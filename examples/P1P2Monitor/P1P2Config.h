/* P1P2Config.h
 *
 * Hitachi H-LINK version with CRC_GEN = 00, and increased buffer sizes
 *
 * Copyright (c) 2019-2022 Arnold Niessen, arnold.niessen-at-gmail-dot-com - licensed under CC BY-NC-ND 4.0 with exceptions (see LICENSE.md)
 *
 * 20221129 v0.9.28 option to insert message in 40F030 time slot for restart or user-defined write message
 * 20221102 v0.9.24 suppress repeated "too long" warnings
 * 20221029 v0.9.23 ADC code, fix 'W' command, misc
 * 20220918 v0.9.22 scopemode for writes and focused on actual errors, fake error generation for test purposes, control for FDY/FDYQ (#define F_SERIES), L2/L3/L5 mode
 * 20220903 v0.9.19 minor change in serial output
 * 20220830 v0.9.18 version alignment for firmware image release
 * 20220819 v0.9.17-fix fixed non-functioning 'E'/'n' write commands
 * 20220817 v0.9.17 minor changes, fixed 'W' command handling magic string prefix, SERIALSPEED/OLDP1P2LIB in P1P2Config.h now depends on F_CPU/COMBIBOARD selection
 * 20220808 v0.9.15 Added compile date/time to verbosity report
 * 20220802 v0.9.14 New parameter-write method 'e' (param write packet type 35-3D), error and write throttling, verbosity mode 2, EEPROM state, MCUSR reporting,
 *                  pseudo-packets, error counters, ...
 *                  Simplified code by removing packet interpretation, json, MQTT, UDP (all off-loaded to P1P2MQTT)
 *
 */

// json/MQTT support has been removed, as it is better to do so on a separate CPU
// Code/data size on an ATmega328P:

                         // prog-size data-size    Function
                         //      kB        kB
                         //    16.3       0.9      Basic functionality
#define EEPROM_SUPPORT   //     0.5       0        adds EEPROM support to store verbose, counterrepeatingrequest, and CONTROL_ID
#define PSEUDO_PACKETS   //     0.9       0        adds pseudopacket to serial output with ATmega status info for P1P2-bridge-esp8266

                         // ------------------
                         //    20.3       0.9      ATmega328P/Arduino Uno

// Define serial speed
// Use 115200 for Arduino Uno/Mega2560 (serial over USB)
// Use 250000 for P1P2-ESP-interface, for direct serial connection between ATmega and ESP8266
//
#define COMBIBOARD // define this for Uno+ESP combiboard, determines SERIALSPEED (250k instead of 115k2) and SERIAL_MAGICSTRING. Ignored if F_CPU=8MHz (as used in P1P2-ESP-Interface)

#if F_CPU > 8000000L
// Assume Arduino Uno (or Mega) hardware; use 115200 Baud for USB or 250000 Baud for combi-board
#ifdef COMBIBOARD
#define SERIALSPEED 250000
#define SERIAL_MAGICSTRING "1P2P" // Serial input line should start with SERIAL_MAGICSTRING, otherwise input line is ignored
#else
#define SERIALSPEED 115200
// do not #define SERIAL_MAGICSTRING
#endif
#else /* F_CPU <= 8MHz */
// Assume on P1P2-ESP-Interface hardware, 8MHz ATmega328P(B), use 250000 Baud and SERIAL_MAGICSTRING
#define SERIALSPEED 250000
#define SERIAL_MAGICSTRING "1P2P" // Serial input line should start with SERIAL_MAGICSTRING, otherwise input line is ignored
#endif /* F_CPU */

#define WELCOMESTRING "* P1P2Monitor-v0.9.28-for-H-link"

#define INIT_VERBOSE 3
// Set verbosity level
// verbose = 0: very limited reporting, raw data only (no R prefix); other data starts with *
// verbose = 1: interactive behavior, maximal reporting (*/R prefix)
// verbose = 2: status reports via pseudo packet, limited reporting, used in P1P2-ESP-interface (*/R prefix) for P1P2-bridge-esp8266/P1P2MQTT (default)
// verbose = 3: as verbose 2, but with timing info added (also works with P1P2-bridge-esp8266/P1P2MQTT) for real packets
//                   (format: 10-character "T 65.535: " for real packets and "P         " for pseudopackets)
// verbose = 4: no raw/pseudopacket data output, only maximal reporting

// EEPROM saves state of
// -verbose (verbosity level)
//
// to reset EEPROM to settings in P1P2Config.h, either erase EEPROM, or change EEPROM_SIGNATURE in P1P2Config.h

#define EEPROM_SIGNATURE "P1P2SIG01" // change this every time you wish to re-init EEPROM settings to the settings in P1P2Config.h
#define EEPROM_ADDRESS_CONTROL_ID      0x00 // 1 byte for CONTROL_ID (Daikin-specific, can be reused)
#define EEPROM_ADDRESS_COUNTER_STATUS  0x01 // 1 byte for counterrepeatingrequest (Daikin-specific, can be reused)
#define EEPROM_ADDRESS_VERBOSITY       0x02 // 1 byte for verbose
                                            // 0x03 .. 0x0F reserved
#define EEPROM_ADDRESS_SIGNATURE       0x10 // should be highest, in view of unspecified strlen(EEPROM_SIGNATURE)

// serial read buffer size for reading from serial port, max line length on serial input is 150 (2 characters per byte, plus some)
#define RS_SIZE 150
// P1/P2 write buffer size for writing to P1P2bus, max packet size is 32 (have not seen anytyhing over 24 (23+CRC))
#define WB_SIZE 65
// P1/P2 read buffer size to store raw data and error codes read from P1P2bus; 1 extra for reading back CRC byte; 24 might be enough
#define RB_SIZE 65

#define INIT_ECHO 1         // defines whether written data is read back and verified against written data (advise to keep this 1)
#define INIT_SCOPE 0        // defines whether scopemode, recording timing info, is on/off at start (advise to keep this 0)

#define INIT_SD 50        // (uint16_t) delay setting in ms for each manually instructed packet write
#define INIT_SDTO 2500    // (uint16_t) time-out delay in ms (applies both to manual instructed writes and controller writes)

// CRC settings, values can be changed via serial port
#define CRC_GEN 0x00    // Default generator/Feed for CRC check; these values work at least for the Daikin hybrid
#define CRC_FEED 0x00   // Define CRC_GEN to 0x00 means no CRC is checked when reading or added when writing
