**P1P2Monitor commands and serial output format**

These commands and reporting structures may change in later versions, especially the command to change parameters on the main controller.

The following commands can be given to P1P2Monitor via the mqtt channel P1P2/W (responses via P1P2/R) on the ESP8266 running P1P2-bridge-esp8266, or directly on the serial interface of the ATmega328P. Mqtt commands can be given by "mosquitto_pub -h <host> [-p <portnr>] -t P1P2/W -m <command>.

The most relevant commands are:
-L1 start auxiliary controller
-C2 start requesting counters
-Z0 set heating(/cooling) off
-Z1 set heating(/cooling) on
-Z/R/N/P/Q/M commands to set other parameters - *parameter change command structure will be redesigned soon to make changes to parameter settings easier.*

P1P2Monitor commands are case-insensitive. The maximum command line length is 98 bytes, long lines will be ignored.

*Warning: The first command line received by P1P2Monitor after its reboot will - on purpose - be ignored for robustness reasons. A good habit is to send a dummy line "\*" as first command. In a future release P1P2-bridge-esp8266 will take care of this so this warning only applies to the current and older version.*

**Serial output format:**

P1P2Monitor output always starts with "R" for valid data read from the P1/P2 bus, or with a "\* " for any other output (including invalid data with read errors). On MQTT these are communicated over P1P2/R and P1P2/S topics. The P1P2/S is meant to be human-readable, the P1P2/R communication is mostly raw hex data, subject to verbosity setting prefixed by a relative time stamp and postfixed by a CRC byte.

In addition, if JSON and OUTPUTSERIAL are defined, json parameter blocks are output over serial on lines starting with "J ". If MQTTTOPICS and OUTPUTSERIAL are defined, mqtt-like parameters are output on lines starting with "P ". If P1P2-bridge-esp8266 is used, it converts raw hex data to json format via P1P2/J or in mqtt (topic=parameter, msg=value) form via P1P2/P.

**Monitor commands:**

- Ux Sets mode off/on (0/1) whether to include (in json format and on ESP also on mqtt) unknown bits and bytes (default off, configurable in P1P2Config.h)
- U  Display display-unknown mode
- Sx Sets mode off/on (0/1) whether to include (in json format and on ESP also on mqtt) parameters only if changed (default on, configurable in P1P2Config.h)
- S  Display changed-only mode
- V  Show verbosity mode (default 1) (default 1 in P1P2Monitor.cpp) 
- Vx Sets verbosity mode (more verbosity levels may be introduced in a later version)
- \* comment lines starting with an asterisk are ignored (and echoed in verbose mode)

**Auxiliary controller commands:**

- L1 sets auxiliary controller mode on (controller address (0xF0 or 0xF1) is auto-detected after check whether another auxiliary controller is present or not) (can also be set in P1P2Config.h)
- L0 sets auxiliary controller mode off
- L  displays current controller_id (0x00 = off; 0xF0/0xF1 is first/secondary auxiliary controller)
- Zx sets heating(/cooling) on/off (1/0) (function can be changed using Px command below to set any 8-bit parameter in packet type 35)
- Rx sets DHW temp (function can be changed using Qx command below to set any 16-bit parameter in packet type 36)
- Nx sets 8-bit value of 8-bit parameter selected by Q command) in packet type 3A
- Px sets (16-bit) parameter number in packet type 35 to use for Zx command (default PARAM_HC_ONOFF)
- Qx sets (16-bit) parameter number in packet type 36 to use for Rx command (default PARAM_TEMP)
- Mx sets (16-bit) parameter number in packet type 3A to use for Nx command (default PARAM_SYS)
- C1 triggers single cycle of 6 B8 packets to request (energy/operation/starts) counters from heat pump
- C2 like C1, but repeating every new minute (can be made default by setting COUNTERREPEATINGREQUEST to 1 in P1P2Config.h)
- C0 Stop requesting counters
- C  Show counterrepeatingrequest status
- BX increment write budget by 10 (or other value as configured in header file, not yet released, planned for future release)
- B  show write budget (planned for future release)

**Raw data commands:**

Commands for raw data writing to the bus (only for reverse engineering purposes), avoid these unless you know what you do:
- W\<hex data\> Write packet (max 32 bytes (as defined by WB_SIZE)) (no 0x prefix should be used for the hex bytes; hex bytes may be concatenated or separated by white space)
- T  Display current delay value (a packet will be written after exactly <delay> ms after the latest start bit, or if the bus has been silent for <delaytimeout> ms)
- Tx sets new delay value in ms, to be used for future packets (default 50 (older versions: 0))
- O  Display current delaytimeout value
- Ox sets new delay timeout value in ms, to be used immediately (default 2500)

**Miscellaneous**

Still or to be supported but advised not to use, not really needed (some may be removed in a future version):
- G  Display current crc_gen value
- Gx Sets crc_gen (default 0xD9) (we have not seen any other values so no need to change)
- H  Display current crc_feed value
- Hx Sets crc_feed (default 0x00) (we have not seen any other values so no need to change)
- X  Display current echo status (determines whether bytes written will be echoed on the serial line, and also whether bus errors will be detected during writing)
- Xx sets echo status on/off (recommended to keep on to detect bus errors)
- Yx sets DHW on/off (use Px/Rx commands instead)
- Y reports DHW status as reported by main controller
- Z reports heating/cooling status as reported by main controller
- P reports parameter number in packet type 35 used for cooling/heating on/off
- K soft reset ATmega328P
- A instruct ESP to reset ATmega328P via reset line (to be released soon)
- D instruct ESP to reset itself (to be released soon)
- E reserved
- F reserved
- I reserved
- J reserved