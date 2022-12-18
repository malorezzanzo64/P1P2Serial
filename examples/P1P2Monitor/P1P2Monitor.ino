/* P1P2Monitor: monitor and control program for HBS such as P1/P2 bus on many Daikin systems.
 *              Reads the P1/P2 bus using the P1P2Serial library, and outputs raw hex data and verbose output over serial.
 *              A few additional parameters are added in raw hex data as so-called pseudo-packets (packet type 0x08 and 0x09).
 *
 *              P1P2Monitor can act as a 2nd (external/auxiliary) controller to the main controller (like an EKRUCBL)
 *               and can use the P1P2 protocol to set various parameters in the main controller, as if these were set manually.
 *              such as heating on/off, DHW on/off, DHW temperature, temperature deviation, etcetera.
 *              It can receive instructions over serial (from an ESP or via USB).
 *              Control over UDP is not yet supported here, but is supported by https://github.com/budulinek/Daikin-P1P2---UDP-Gateway
 *              It can regularly request the system counters to better monitor efficiency and operation.
 *              Control has been tested on EHYHBX08AAV3 and EHVX08S23D6V, and it likely works on various other Altherma models.
 *              The P1P2Monitor command format is described in P1P2Monitor-commands.md,
 *              but just as an example, I can switch my heat pump off with the following commands
 *
 *              *            < one empty line (only needed immediately after reboot, to flush the input buffer) >
 *              L1           < switch control mode on >
 *              Z0           < switch heating off >
 *              Z1           < switch heating on >
 *
 *              That is all!
 *
 *              If you know the parameter number for a certain function
 *              (see https://github.com/budulinek/Daikin-P1P2---UDP-Gateway/blob/main/Payload-data-write.md),
 *              you can set that parameter by:
 *
 *              Q03         < select writing to parameter 0x03 in F036 block, used for DHW temperature setting >
 *              R0208       < set parameter 0x0003 to 0x0208, which is 520, which means 52.0 degrees Celcius >
 *
 *              P40         < select writing to parameter 0x40 in F035 block, used for DHW on/off setting >
 *              Z01         < set parameter 0x0040 to 0x01, DHW status on >
 *              P2F         < select writing back to parameter 0x2F in F035 block, used for heating(/cooloing) on/off >
 *              Z01         < set parameter 0x002F to 0x01, heating status on >
 *
 *              Support for parameter setting in P1P2Monitor is rather simple. There is no buffering, so enough time (a few seconds)
 *              is needed in between commands.
 *
 * Copyright (c) 2019-2022 Arnold Niessen, arnold.niessen-at-gmail-dot-com - licensed under CC BY-NC-ND 4.0 with exceptions (see LICENSE.md)
 *
 * Version history
 *
 * Hitachi H-link cleaned version, increased buffer size
 *
 * 20221129 v0.9.28 option to insert message in 40F030 time slot for restart or user-defined write message
 * 20221102 v0.9.24 suppress repeated "too long" warnings
 * 20221029 v0.9.23 ADC code, fix 'W' command, misc
 * 20220918 v0.9.22 scopemode for writes and focused on actual errors, fake error generation for test purposes, control for FDY/FDYQ (#define F_SERIES), L2/L3/L5 mode
 * 20220903 v0.9.19 minor change in serial output
 * 20220830 v0.9.18 version alignment for firmware image release
 * 20220819 v0.9.17-fix1 fixed non-functioning 'E'/'n' write commands
 * 20220817 v0.9.17 scopemode, fixed 'W' command handling magic string prefix, SERIALSPEED/OLDP1P2LIB in P1P2Config.h now depends on F_CPU/COMBIBOARD selection, ..
 * 20220810 v0.9.15 Improved uptime handling
 * 20220802 v0.9.14 New parameter-write method 'e' (param write packet type 35-3D), error and write throttling, verbosity mode 2, EEPROM state, MCUSR reporting,
 *                  pseudo-packets, error counters, ...
 *                  Simplified code by removing packet interpretation, json, MQTT, UDP (all off-loaded to P1P2MQTT)
 * 20220511 v0.9.13 Removed dependance on millis(), reducing blocking due to serial input, correcting 'Z' handling to hexadecimal, ...
 * 20220225 v0.9.12 Added parameter support for F036 parameters
 * 20200109 v0.9.11 Added option to insert counter request after 000012 packet (for KLIC-DA)
 * 20191018 v0.9.10 Added MQTT topic like output over serial/udp, and parameter support for EHVX08S26CA9W (both thanks to jarosolanic)
 * 20190914 v0.9.9 Controller/write safety related improvements; automatic controller ID detection
 * 20190908 v0.9.8 Minor improvements: error handling, hysteresis, improved output (a.o. x0Fx36)
 * 20190829 v0.9.7 Improved Serial input handling, added B8 counter request
 * 20190828        Fused P1P2Monitor and P1P2json-mega (with udp support provided by Budulinek)
 * 20190827 v0.9.6 Added limited control functionality, json conversion files merged with esp8266 files
 * 20190820 v0.9.5 Changed delay behaviour, timeout added
 * 20190817 v0.9.4 Brought in line with library 0.9.4 (API changes), print last byte if crc_gen==0, added json support
 *                 See comments below for description of serial protocol
 * 20190505 v0.9.3 Changed error handling and corrected deltabuf type in readpacket; added mod12/mod13 counters
 * 20190428 v0.9.2 Added setEcho(b), readpacket() and writepacket(); LCD support added
 * 20190409 v0.9.1 Improved setDelay()
 * 20190407 v0.9.0 Improved reading, writing, and meta-data; added support for timed writings and collision detection; added stand-alone hardware-debug mode
 * 20190303 v0.0.1 initial release; support for raw monitoring and hex monitoring
 *
 *     Thanks to Krakra for providing coding suggestions and UDP support, the hints and references to the HBS and MM1192 documents on
 *     https://community.openenergymonitor.org/t/hack-my-heat-pump-and-publish-data-onto-emoncms,
 *     to Luis Lamich Arocas for sharing logfiles and testing the new controlling functions for the EHVX08S23D6V,
 *     and to Paul Stoffregen for publishing the AltSoftSerial library which was the starting point for the P1P2Serial library.
 *
 * P1P2erial is written and tested for the Arduino Uno and (in the past) Mega.
 * It may or may not work on other hardware using a 16MHz clok.
 * As of P1P2Serial library 0.9.14, it also runs on 8MHz ATmega328P using MCUdude's MiniCore.
 * Information on MCUdude's MiniCore:
 * Install in Arduino IDE (File/preferences, additional board manager URL: https://mcudude.github.io/MiniCore/package_MCUdude_MiniCore_index.json.
 *
 * Board          Transmit  Receive   Unusable PWM pins
 * -----          --------  -------   ------------
 * Arduino Uno        9         8         10
 * Arduino Mega      46        48       44, 45
 * ATmega328P        PB1      PB0
 *
 * Wishlist:
 * -schedule writes
 * -continue control operating (L1) immediately after reset/brown-out, but not immediately after power-up reset
 */

#include "P1P2Config.h"
#include <P1P2Serial.h>

#define SPI_CLK_PIN_VALUE (PINB & 0x20)

static byte verbose = INIT_VERBOSE;
static byte readErrors = 0;
static byte readErrorLast = 0;
static byte errorsLargePacket = 0;

const byte Compile_Options = 0 // multi-line statement
#ifdef PSEUDO_PACKETS
+0x04
#endif
#ifdef EEPROM_SUPPORT
+0x08
#endif
#ifdef SERIAL_MAGICSTRING
+0x10
#endif
;

#ifdef EEPROM_SUPPORT
#include <EEPROM.h>
void initEEPROM() {
  if (verbose) Serial.println(F("* checking EEPROM"));
  bool sigMatch = 1;
  for (uint8_t i = 0; i < strlen(EEPROM_SIGNATURE); i++) sigMatch &= (EEPROM.read(EEPROM_ADDRESS_SIGNATURE + i) == EEPROM_SIGNATURE[i]);
  if (verbose) {
     Serial.print(F("* EEPROM sig match"));
     Serial.println(sigMatch);
  }
  if (!sigMatch) {
    if (verbose) Serial.println(F("* EEPROM sig mismatch, initializing EEPROM"));
    for (uint8_t i = 0; i < strlen(EEPROM_SIGNATURE); i++) EEPROM.update(EEPROM_ADDRESS_SIGNATURE + i, EEPROM_SIGNATURE[i]); // no '\0', not needed
    // Daikin specific EEPROM.update(EEPROM_ADDRESS_CONTROL_ID, CONTROL_ID_DEFAULT);
    // Daikin specific EEPROM.update(EEPROM_ADDRESS_COUNTER_STATUS, COUNTERREPEATINGREQUEST);
    EEPROM.update(EEPROM_ADDRESS_VERBOSITY, INIT_VERBOSE);
  }
}
#define EEPROM_update(x, y) { EEPROM.update(x, y); };
#else /* EEPROM_SUPPORT */
#define EEPROM_update(x, y) {}; // dummy function to avoid cluttering code with #ifdef EEPROM_SUPPORT
#endif /* EEPROM_SUPPORT */

P1P2Serial P1P2Serial;

static uint16_t sd = INIT_SD;           // delay setting for each packet written upon manual instruction (changed to 50ms which seems to be a bit safer than 0)
static uint16_t sdto = INIT_SDTO;       // time-out delay (applies both to manual instructed writes and controller writes)
static byte echo = INIT_ECHO;           // echo setting (whether written data is read back)
static byte allow = ALLOW_PAUSE_BETWEEN_BYTES;
static byte scope = INIT_SCOPE;         // scope setting (to log timing info)
static byte counterRequest = 0;
static byte counterRepeatingRequest = 0;
static byte writeRefused = 0;

byte save_MCUSR;

byte Tmin = 0;
byte Tminprev = 61;
int32_t upt_prev_pseudo = 0;
int32_t upt_prev_write = 0;
int32_t upt_prev_error = 0;

static char RS[RS_SIZE];
static byte WB[WB_SIZE];
static byte RB[RB_SIZE];
static errorbuf_t EB[RB_SIZE];

// serial-receive using direct access to serial read buffer RS
// rs is # characters in buffer;
// sr_buffer is pointer (RS+rs)

static uint8_t rs=0;
static char *RSp = RS;
static byte hwID = 0;

void setup() {
  save_MCUSR = MCUSR;
  MCUSR = 0;
  hwID = SPI_CLK_PIN_VALUE ? 0 : 1;
  Serial.begin(SERIALSPEED);
  while (!Serial);      // wait for Arduino Serial Monitor to open
  Serial.println(F("*"));
  Serial.println(F(WELCOMESTRING));
  Serial.print(F("* Compiled "));
  Serial.print(F(__DATE__));
  Serial.print(F(" "));
  Serial.print(F(__TIME__));
  Serial.println();
  Serial.print(F("* P1P2-ESP-interface hwID "));
  Serial.println(hwID);
#ifdef EEPROM_SUPPORT
  initEEPROM();
  verbose    = EEPROM.read(EEPROM_ADDRESS_VERBOSITY);
#endif /* EEPROM_SUPPORT */
  if (verbose) {
    Serial.print(F("* Verbosity="));
    Serial.println(verbose);
    Serial.print(F("* Reset cause: MCUSR="));
    Serial.print(save_MCUSR);
    if (save_MCUSR & (1 << BORF))  Serial.print(F(" (brown-out-detected)")); // 4
    if (save_MCUSR & (1 << EXTRF)) Serial.print(F(" (ext-reset)")); // 2
    if (save_MCUSR & (1 << PORF)) Serial.print(F(" (power-on-reset)")); // 1
    Serial.println();
// Initial parameters
    Serial.print(F("* CPU_SPEED="));
    Serial.println(F_CPU);
    Serial.print(F("* SERIALSPEED="));
    Serial.println(SERIALSPEED);
    Serial.println();
#ifdef OLDP1P2LIB
    Serial.println(F("* OLDP1P2LIB"));
#else
    Serial.println(F("* NEWP1P2LIB"));
#endif
  }
  P1P2Serial.begin(9600, hwID ? true : false, 6, 7); // if hwID = 1, use ADC6 and ADC7
  P1P2Serial.setEcho(echo);
#ifdef SW_SCOPE
  P1P2Serial.setScope(scope);
#endif
  P1P2Serial.setDelayTimeout(sdto);
  Serial.println(F("* Ready setup"));
}

// scanint and scanhex are used to save dynamic memory usage in main loop
int scanint(char* s, uint16_t &b) {
  return sscanf(s,"%d",&b);
}

int scanhex(char* s, uint16_t &b) {
  return sscanf(s,"%4x",&b);
}

static byte crc_gen = CRC_GEN;
static byte crc_feed = CRC_FEED;

void writePseudoPacket(byte* WB, byte rh)
{
  if (verbose) Serial.print(F("R "));
  if (verbose & 0x01) Serial.print(F("P         "));
  uint8_t crc = crc_feed;
  for (uint8_t i = 0; i < rh; i++) {
    uint8_t c = WB[i];
    if (c <= 0x0F) Serial.print(F("0"));
    Serial.print(c, HEX);
    if (crc_gen != 0) for (uint8_t i = 0; i < 8; i++) {
      crc = ((crc ^ c) & 0x01 ? ((crc >> 1) ^ crc_gen) : (crc >> 1));
      c >>= 1;
    }
  }
  if (crc_gen) {
    if (crc <= 0x0F) Serial.print(F("0"));
    Serial.print(crc, HEX);
  }
  Serial.println();
}

void(* resetFunc) (void) = 0; // declare reset function at address 0

static byte pseudo0D = 0;
static byte pseudo0E = 0;
static byte pseudo0F = 0;

void loop() {
  uint16_t temp;
  uint16_t temphex;
  int wb = 0;
  int n;
  int wbtemp;
  int c;
  static byte ignoreremainder = 2; // ignore first line from serial input to avoid misreading a partial message just after reboot
  static bool reportedTooLong = 0;

// Non-blocking serial input
// rs is number of char received and stored in readbuf
// RSp = readbuf + rs
// ignore first line and too-long lines
  while (((c = Serial.read()) >= 0) && (c != '\n') && (rs < RS_SIZE)) {
    *RSp++ = (char) c;
    rs++;
  }
  // ((c == '\n' and rs > 0))  and/or  rs == RS_SIZE)  or  c == -1
  if (c >= 0) {
    if ((c != '\n') || (rs == RS_SIZE)) {
      //  (c != '\n' ||  rb == RB)
      char lst = *(RSp - 1);
      *(RSp - 1) = '\0';
      if (c != '\n') {
        if (!reportedTooLong) {
          Serial.print(F("* Line too long, ignored, ignoring remainder: ->"));
          Serial.print(RS);
          Serial.print(lst);
          Serial.print(c);
          Serial.println(F("<-"));
          // show full line, so not yet setting reportedTooLong = 1 here;
        }
        ignoreremainder = 1;
      } else {
        if (!reportedTooLong) {
          Serial.print(F("* Line too long, terminated, ignored: ->"));
          Serial.print(RS);
          Serial.print(lst);
          Serial.println(F("<-"));
          reportedTooLong = 1;
        }
        ignoreremainder = 0;
      }
    } else {
      // c == '\n'
      *RSp = '\0';
      // Remove \r before \n in DOS input
      if ((rs > 0) && (RS[rs - 1] == '\r')) {
        rs--;
        *(--RSp) = '\0';
      }
      if (ignoreremainder == 2) {
        // Serial.print(F("* First line ignored: ->")); // do not copy - usually contains boot-related nonsense
        // Serial.print(RS);
        // Serial.println("<-");
        ignoreremainder = 0;
      } else if (ignoreremainder == 1) {
        if (!reportedTooLong) {
          Serial.print(F("* Line too long, last part: "));
          Serial.println(RS);
          reportedTooLong = 1;
        }
        ignoreremainder = 0;
      } else {
#define maxVerbose ((verbose == 1) || (verbose == 4))
        if (maxVerbose) {
          Serial.print(F("* Received: \""));
          Serial.print(RS);
          Serial.println("\"");
        }
#ifdef SERIAL_MAGICSTRING
        RSp = RS + strlen(SERIAL_MAGICSTRING) + 1;
        if (!strncmp(RS, SERIAL_MAGICSTRING, strlen(SERIAL_MAGICSTRING))) {
#else
        RSp = RS + 1;
        {
#endif
          switch (*(RSp - 1)) {
            case '\0':if (maxVerbose) Serial.println(F("* Empty line received")); break;
            case '*': if (maxVerbose) {
                        Serial.print(F("* Received: "));
                        Serial.println(RSp);
                      }
                      break;
            case 'g':
            case 'G': if (verbose) Serial.print(F("* Crc_gen "));
                      if (scanhex(RSp, temphex) == 1) {
                        crc_gen = temphex;
                        if (!verbose) break;
                        Serial.print(F("set to "));
                      }
                      Serial.print(F("0x"));
                      if (crc_gen <= 0x0F) Serial.print(F("0"));
                      Serial.println(crc_gen, HEX);
                      break;
            case 'h':
            case 'H': if (verbose) Serial.print(F("* Crc_feed "));
                      if (scanhex(RSp, temphex) == 1) {
                        crc_feed = temphex;
                        if (!verbose) break;
                        Serial.print(F("set to "));
                      }
                      Serial.print(F("0x"));
                      if (crc_feed <= 0x0F) Serial.print(F("0"));
                      Serial.println(crc_feed, HEX);
                      break;
            case 'v':
            case 'V': Serial.print(F("* Verbose "));
                      if (scanint(RSp, temp) == 1) {
                        if (temp > 4) temp = 4;
                        Serial.print(F("set to "));
                        verbose = temp;
                        EEPROM_update(EEPROM_ADDRESS_VERBOSITY, verbose);
                      }
                      Serial.println(verbose);
                      Serial.println(F(WELCOMESTRING));
                      Serial.print(F("* Compiled "));
                      Serial.print(F(__DATE__));
                      Serial.print(F(" "));
                      Serial.println(F(__TIME__));
#ifdef OLDP1P2LIB
                      Serial.print(F("* OLDP1P2LIB"));
#else
                      Serial.print(F("* NEWP1P2LIB"));
#endif
                      Serial.print(F("* Reset cause: MCUSR="));
                      Serial.print(save_MCUSR);
                      if (save_MCUSR & (1 << BORF))  Serial.print(F(" (brown-out-detected)")); // 4
                      if (save_MCUSR & (1 << EXTRF)) Serial.print(F(" (ext-reset)")); // 2
                      if (save_MCUSR & (1 << PORF)) Serial.print(F(" (power-on-reset)")); // 1
                      Serial.println();
                      Serial.print(F("* P1P2-ESP-Interface hwID "));
                      Serial.println(hwID);
                      break;
            case 't':
            case 'T': if (verbose) Serial.print(F("* Delay "));
                      if (scanint(RSp, sd) == 1) {
                        if (sd < 2) {
                          sd = 2;
                          Serial.print(F("[use of delay 0 or 1 not recommended, increasing to 2] "));
                        }
                        if (!verbose) break;
                        Serial.print(F("set to "));
                      }
                      Serial.println(sd);
                      break;
            case 'o':
            case 'O': if (verbose) Serial.print(F("* DelayTimeout "));
                      if (scanint(RSp, sdto) == 1) {
                        P1P2Serial.setDelayTimeout(sdto);
                        if (!verbose) break;
                        Serial.print(F("set to "));
                      }
                      Serial.println(sdto);
                      break;
#ifdef SW_SCOPE
            case 'u':
            case 'U': if (verbose) Serial.print(F("* Software-scope "));
                      if (scanint(RSp, temp) == 1) {
                        scope = temp;
                        if (scope > 1) scope = 1;
                        P1P2Serial.setScope(scope);
                        if (!verbose) break;
                        Serial.print(F("set to "));
                      }
                      Serial.println(scope);
                      break;
#endif
            case 'x':
            case 'X': if (verbose) Serial.print(F("* Allow (bits pause) "));
                      if (scanint(RSp, temp) == 1) {
                        allow = temp;
                        if (allow > 76) allow = 76;
                        P1P2Serial.setAllow(allow);
                        if (!verbose) break;
                        Serial.print(F("set to "));
                      }
                      Serial.println(allow);
                      break;
            case 'w':
            case 'W': if (verbose) Serial.print(F("* Writing: "));
                      wb = 0;
                      while ((wb < WB_SIZE) && (sscanf(RSp, (const char*) "%2x%n", &wbtemp, &n) == 1)) {
                        WB[wb++] = wbtemp;
                        RSp += n;
                        if (verbose) {
                          if (wbtemp <= 0x0F) Serial.print("0");
                          Serial.print(wbtemp, HEX);
                        }
                      }
                      if (verbose) Serial.println();
                      if (P1P2Serial.writeready()) {
                        if (wb) {
                          P1P2Serial.writepacket(WB, wb, sd, crc_gen, crc_feed);
                        } else {
                          Serial.println(F("* Refusing to write empty packet"));
                        }
                      } else {
                        Serial.println(F("* Refusing to write packet while previous packet wasn't finished"));
                        if (writeRefused < 0xFF) writeRefused++;
                      }
                      break;
            case 'k': // reset ATmega
            case 'K': Serial.println(F("* Resetting ...."));
                      resetFunc(); //call reset
            default:  Serial.print(F("* Command not understood: "));
                      Serial.println(RSp - 1);
                      break;
          }
#ifdef SERIAL_MAGICSTRING
/*
        } else {
          if (!strncmp(RS, "* [ESP]", 7)) {
            Serial.print(F("* Ignoring: "));
          } else {
            Serial.print(F("* Magic String mismatch: "));
          }
          Serial.println(RS);
*/
#endif
        }
      }
    }
    RSp = RS;
    rs = 0;
  }
  int32_t upt = P1P2Serial.uptime_sec();
  if (upt > upt_prev_pseudo) {
    pseudo0D++;
    pseudo0E++;
    pseudo0F++;
    upt_prev_pseudo = upt;
  }
  while (P1P2Serial.packetavailable()) {
    uint16_t delta;
    errorbuf_t readError = 0;
    int nread = P1P2Serial.readpacket(RB, delta, EB, RB_SIZE, crc_gen, crc_feed);
    if (nread > RB_SIZE) {
      Serial.println(F("* Received packet longer than RB_SIZE"));
      nread = RB_SIZE;
      readError = 0xFF;
      if (errorsLargePacket < 0xFF) errorsLargePacket++;
    }
    for (int i = 0; i < nread; i++) readError |= EB[i];
#ifdef SW_SCOPE

#if F_CPU > 8000000L
#define FREQ_DIV 4 // 16 MHz
#else
#define FREQ_DIV 3 // 8 MHz
#endif

    if (scope) {
      if (sws_cnt || (sws_event[SWS_MAX - 1] != SWS_EVENT_LOOP)) {
        if (readError) {
          Serial.print(F("C "));
        } else {
          Serial.print(F("c "));
        }
        if (RB[0] < 0x10) Serial.print('0');
        Serial.print(RB[0], HEX);
        if (RB[1] < 0x10) Serial.print('0');
        Serial.print(RB[1], HEX);
        if (RB[2] < 0x10) Serial.print('0');
        Serial.print(RB[2], HEX);
        Serial.print(' ');
        static uint16_t capture_prev;
        int i = 0;
        if (sws_event[SWS_MAX - 1] != SWS_EVENT_LOOP) i = sws_cnt;
        bool skipfirst = true;
        do {
          switch (sws_event[i]) {
            // error events
            case SWS_EVENT_ERR_BE      : Serial.print(F("   BE   ")); break;
            case SWS_EVENT_ERR_BE_FAKE : Serial.print(F("   be   ")); break;
            case SWS_EVENT_ERR_SB      : Serial.print(F("   SB   ")); break;
            case SWS_EVENT_ERR_SB_FAKE : Serial.print(F("   sb   ")); break;
            case SWS_EVENT_ERR_BC      : Serial.print(F("   BC   ")); break;
            case SWS_EVENT_ERR_BC_FAKE : Serial.print(F("   bc   ")); break;
            case SWS_EVENT_ERR_PE      : Serial.print(F("   PE   ")); break;
            case SWS_EVENT_ERR_PE_FAKE : Serial.print(F("   pe   ")); break;
            case SWS_EVENT_ERR_LOW     : Serial.print(F("   lw   ")); break;
            // read/write related events
            default   : byte sws_ev    = sws_event[i] & SWS_EVENT_MASK;
                        byte sws_state = sws_event[i] & 0x1F;
                        if (!skipfirst) {
                          if (sws_ev == SWS_EVENT_EDGE_FALLING_W) Serial.print(' ');
                          uint16_t t_diff = (sws_capture[i] - capture_prev) >> FREQ_DIV;
                          if (t_diff < 10) Serial.print(' ');
                          if (t_diff < 100) Serial.print(' ');
                          Serial.print(t_diff);
                          if (sws_ev != SWS_EVENT_EDGE_FALLING_W) Serial.print(':');
                        }
                        capture_prev = sws_capture[i];
                        skipfirst = false;
                        switch (sws_ev) {
                          case SWS_EVENT_SIGNAL_LOW     : Serial.print('?'); break;
                          case SWS_EVENT_EDGE_FALLING_W : Serial.print(' '); break;
                          case SWS_EVENT_EDGE_RISING    :
                                                          // state = 1 .. 20 (1,2 start bit, 3-18 data bit, 19,20 parity bit),
                                                          switch (sws_state) {
                                                            case 1       :
                                                            case 2       : Serial.print('S'); break; // start bit
                                                            case 3 ... 18: Serial.print((sws_state - 3) >> 1); break; // state=3-18 for data bit 0-7
                                                            case 19      :
                                                            case 20      : Serial.print('P'); break; // parity bit
                                                            default      : Serial.print(' '); break;
                                                          }
                                                          break;
                          case SWS_EVENT_EDGE_SPIKE     :
                          case SWS_EVENT_SIGNAL_HIGH_R  :
                          case SWS_EVENT_EDGE_FALLING_R :
                                                          switch (sws_state) {
                                                            case 11      : Serial.print('E'); break; // stop bit ('end' bit)
                                                            case 10      : Serial.print('P'); break; // parity bit
                                                            case 0       :
                                                            case 1       : Serial.print('S'); break; // parity bit
                                                            case 2 ... 9 : Serial.print(sws_state - 2); break; // state=2-9 for data bit 0-7
                                                            default      : Serial.print(' '); break;
                                                          }
                                                          break;
                          default                       : Serial.print(' ');
                                                          break;
                        }
                        switch (sws_ev) {
                          case SWS_EVENT_EDGE_SPIKE     : Serial.print(F("-X-"));  break;
                          case SWS_EVENT_SIGNAL_HIGH_R  : Serial.print(F("‾‾‾")); break;
                          case SWS_EVENT_SIGNAL_LOW     : Serial.print(F("___")); break;
                          case SWS_EVENT_EDGE_RISING    : Serial.print(F("_/‾")); break;
                          case SWS_EVENT_EDGE_FALLING_W :
                          case SWS_EVENT_EDGE_FALLING_R : Serial.print(F("‾\\_")); break;
                          default                       : Serial.print(F(" ? ")); break;
                        }
          }
          if (++i == SWS_MAX) i = 0;
        } while (i != sws_cnt);
        Serial.println();
      }
    }
    sws_block = 0; // release SW_SCOPE for next log operation
#endif
#ifdef MEASURE_LOAD
    if (irq_r) {
      uint8_t irq_busy_c1 = irq_busy;
      uint16_t irq_lapsed_r_c = irq_lapsed_r;
      uint16_t irq_r_c = irq_r;
      uint8_t  irq_busy_c2 = irq_busy;
      if (irq_busy_c1 || irq_busy_c2) {
        Serial.println(F("* irq_busy"));
      } else {
        Serial.print(F("* sh_r="));
        Serial.println(100.0 * irq_r_c / irq_lapsed_r_c);
      }
    }
    if (irq_w) {
      uint8_t irq_busy_c1 = irq_busy;
      uint16_t irq_lapsed_w_c = irq_lapsed_w;
      uint16_t irq_w_c = irq_w;
      uint8_t  irq_busy_c2 = irq_busy;
      if (irq_busy_c1 || irq_busy_c2) {
        Serial.println(F("* irq_busy"));
      } else {
        Serial.print(F("* sh_w="));
        Serial.println(100.0 * irq_w_c / irq_lapsed_w_c);
      }
    }
#endif
    if ((readError & 0xBB & ERROR_REAL_MASK) && upt) { // don't count errors while upt == 0  // suppress PE and UC errors for H-link
      if (readErrors < 0xFF) {
        readErrors++;
        readErrorLast = readError;
      }
    }
// for H-link2, split reporting errors (if any) and data (always)
    uint16_t delta2 = delta;
    if (readError) {
      Serial.print(F("E "));
      // 3nd-12th characters show length of bus pause (max "R T 65.535: ")
      Serial.print(F("T "));
      if (delta < 10000) Serial.print(F(" "));
      if (delta < 1000) Serial.print(F("0")); else { Serial.print(delta / 1000); delta %= 1000; };
      Serial.print(F("."));
      if (delta < 100) Serial.print(F("0"));
      if (delta < 10) Serial.print(F("0"));
      Serial.print(delta);
      Serial.print(F(": "));
      for (int i = 0; i < nread; i++) {
        if (verbose && (EB[i] & ERROR_SB)) {
          // collision suspicion due to data verification error in reading back written data
          Serial.print(F("-SB:"));
        }
        if (verbose && (EB[i] & ERROR_BE)) { // or BE3 (duplicate code)
          // collision suspicion due to data verification error in reading back written data
          Serial.print(F("-XX:"));
        }
        if (verbose && (EB[i] & ERROR_BC)) {
          // collision suspicion due to 0 during 2nd half bit signal read back
          Serial.print(F("-BC:"));
        }
        if (verbose && (EB[i] & ERROR_PE)) {
          // parity error detected
          Serial.print(F("-PE:"));
        }
        if (verbose && (EB[i] & SIGNAL_UC)) {
          // parity error detected
          Serial.print(F("-UC:"));
        }
#ifdef GENERATE_FAKE_ERRORS
        if (verbose && (EB[i] & (ERROR_SB << 8))) {
          // collision suspicion due to data verification error in reading back written data
          Serial.print(F("-sb:"));
        }
        if (verbose && (EB[i] & (ERROR_BE << 8))) {
          // collision suspicion due to data verification error in reading back written data
          Serial.print(F("-xx:"));
        }
        if (verbose && (EB[i] & (ERROR_BC << 8))) {
          // collision suspicion due to 0 during 2nd half bit signal read back
          Serial.print(F("-bc:"));
        }
        if (verbose && (EB[i] & (ERROR_PE << 8))) {
          // parity error detected
          Serial.print(F("-pe:"));
        }
#endif
        byte c = RB[i];
        if (crc_gen && (verbose == 1) && (i == nread - 1)) {
          Serial.print(F(" CRC="));
        }
        if (c < 0x10) Serial.print(F("0"));
        Serial.print(c, HEX);
        if (verbose && (EB[i] & ERROR_OR)) {
          // buffer overrun detected (overrun is after, not before, the read byte)
          Serial.print(F(":OR-"));
        }
        if (verbose && (EB[i] & ERROR_CRC)) {
          // CRC error detected in readpacket
          Serial.print(F(" CRC error"));
        }
      }
      if (readError & 0xBB) {
        Serial.print(F(" readError=0x"));
        if (readError < 0x10) Serial.print('0');
        if (readError < 0x100) Serial.print('0');
        if (readError < 0x1000) Serial.print('0');
        Serial.print(readError, HEX);
      }
      Serial.println();
    }
    delta = delta2;
    if (!(readError & 0xBB)) {
      if (verbose && (verbose < 4)) Serial.print(F("R "));
      if ((verbose & 0x01) == 1)  {
        // 3nd-12th characters show length of bus pause (max "R T 65.535: ")
        Serial.print(F("T "));
        if (delta < 10000) Serial.print(F(" "));
        if (delta < 1000) Serial.print(F("0")); else { Serial.print(delta / 1000); delta %= 1000; };
        Serial.print(F("."));
        if (delta < 100) Serial.print(F("0"));
        if (delta < 10) Serial.print(F("0"));
        Serial.print(delta);
        Serial.print(F(": "));
      }
      if (verbose < 4) {
        for (int i = 0; i < nread; i++) {
          byte c = RB[i];
          if (crc_gen && (verbose == 1) && (i == nread - 1)) {
            Serial.print(F(" CRC="));
          }
          if (c < 0x10) Serial.print(F("0"));
          Serial.print(c, HEX);
        }
        Serial.println();
      }
    }
  }
#ifdef PSEUDO_PACKETS
  if (pseudo0D > 4) {
    pseudo0D = 0;
    WB[0]  = 0x00;
    WB[1]  = 0x00;
    WB[2]  = 0x0D;
    if (hwID) {
      uint16_t V0_min;
      uint16_t V0_max;
      uint32_t V0_avg;
      uint16_t V1_min;
      uint16_t V1_max;
      uint32_t V1_avg;
      P1P2Serial.ADC_results(V0_min, V0_max, V0_avg, V1_min, V1_max, V1_avg);
      WB[3]  = (V0_min >> 8) & 0xFF;
      WB[4]  = V0_min & 0xFF;
      WB[5]  = (V0_max >> 8) & 0xFF;
      WB[6]  = V0_max & 0xFF;
      WB[7]  = (V0_avg >> 24) & 0xFF;
      WB[8]  = (V0_avg >> 16) & 0xFF;
      WB[9]  = (V0_avg >> 8) & 0xFF;
      WB[10] = V0_avg & 0xFF;
      WB[11]  = (V1_min >> 8) & 0xFF;
      WB[12] = V1_min & 0xFF;
      WB[13] = (V1_max >> 8) & 0xFF;
      WB[14] = V1_max & 0xFF;
      WB[15] = (V1_avg >> 24) & 0xFF;
      WB[16] = (V1_avg >> 16) & 0xFF;
      WB[17] = (V1_avg >> 8) & 0xFF;
      WB[18] = V1_avg & 0xFF;
      WB[19] = 0x00;
      WB[20] = 0x00;
      WB[21] = 0x00;
      WB[22] = 0x00;
    } else { 
      for (int i = 3; i <= 22; i++) WB[i]  = 0x00;
    }
    if (verbose < 4) writePseudoPacket(WB, 23);
  }
  if (pseudo0E > 4) {
    pseudo0E = 0;
    WB[0]  = 0x00;
    WB[1]  = 0x00;
    WB[2]  = 0x0E;
    WB[3]  = (upt >> 24) & 0xFF;
    WB[4]  = (upt >> 16) & 0xFF;
    WB[5]  = (upt >> 8) & 0xFF;
    WB[6]  = upt & 0xFF;
    WB[7]  = (F_CPU >> 24) & 0xFF;
    WB[8]  = (F_CPU >> 16) & 0xFF;
    WB[9]  = (F_CPU >> 8) & 0xFF;
    WB[10] = F_CPU & 0xFF;
    WB[11] = writeRefused;
    WB[12] = counterRepeatingRequest;
    WB[13] = counterRequest;
    WB[14] = errorsLargePacket;
    WB[15] = 0x00;
    WB[16] = 0x00;
    WB[17] = 0x00;
    WB[18] = 0x00;
    WB[19] = 0x00;
    WB[20] = 0x00;
    WB[21] = 0x00;
    WB[22] = 0x00;
    if (verbose < 4) writePseudoPacket(WB, 23);
  }
  if (pseudo0F > 4) {
    bool sigMatch = 1;
    pseudo0F = 0;
    WB[0]  = 0x00;
    WB[1]  = 0x00;
    WB[2]  = 0x0F;
    WB[3]  = Compile_Options;
    WB[4]  = verbose;
    WB[5]  = save_MCUSR;
    WB[6]  = 0x00;
    WB[7]  = 0x00;
    WB[8]  = 0x00;
    WB[9]  = 0x00;
    WB[10] = (sd >> 8) & 0xFF;
    WB[11] = sd & 0xFF;
    WB[12] = (sdto >> 8) & 0xFF;
    WB[13] = sdto & 0xFF;
    WB[14] = hwID;
#ifdef EEPROM_SUPPORT
    WB[15] = 0x00; // EEPROM.read(EEPROM_ADDRESS_CONTROL_ID);
    WB[16] = EEPROM.read(EEPROM_ADDRESS_VERBOSITY);
    WB[17] = 0x00; // EEPROM.read(EEPROM_ADDRESS_COUNTER_STATUS);
    for (uint8_t i = 0; i < strlen(EEPROM_SIGNATURE); i++) sigMatch &= (EEPROM.read(EEPROM_ADDRESS_SIGNATURE + i) == EEPROM_SIGNATURE[i]);
    WB[18] = sigMatch;
#else
    WB[15] = 0x00;
    WB[16] = 0x00;
    WB[17] = 0x00;
    WB[18] = 0x00;
#endif
    WB[19] = scope;
    WB[20] = readErrors;
    WB[21] = readErrorLast;
    WB[22] = 0x00;
    if (verbose < 4) writePseudoPacket(WB, 23);
  }
#endif /* PSEUDO_PACKETS */
}
