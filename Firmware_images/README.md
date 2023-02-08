# Hitachi firmware images for P1P2MQTT bridge (previously P1P2-ESP-Interface)

## P1P2Monitor firmware for ATmega328 on P1P2MQTT bridge

These images are for the P1P2-ESP-interface v1.0 and v1.1 for experimental use with Hitachi heat pumps. They do not work on the Arduino Uno.

P1P2Monitor on H-link2 only enables monitoring at this moment.

Firmware image: [P1P2Monitor-H_series-v0.9.32Hlink2.ino.hex](P1P2Monitor-H_series-v0.9.32Hlink2.ino.hex)

To install (Linux CLI):

```
avrdude -c avrisp -p atmega328p  -P net:<IPv4>:328 -e -Uflash:w:P1P2Monitor-0.9.24-Eseries.ino.hex:i
```

To install (Windows CLI, using avrdude 7.0 for Windows):

```
avrdude.exe -c avrisp -p m328p  -P net:<IPv4>:328 -e -Uflash:w:P1P2Monitor-0.9.24-Eseries.ino.hex:i
```

## P1P2-bridge-esp8266 firmware for ESP8266 on P1P2MQTT bridge

Firmware image: [P1P2-bridge-esp8266-0.9.32Hlink2-H-series-P1P2MQTT-bridge.ino.bin](P1P2-bridge-esp8266-0.9.32Hlink2-H-series-P1P2MQTT-bridge.ino.bin)

To install OTA (Linux CLI):

```
~/.arduino15/packages/esp8266/tools/python3/3.7.2-post1/python3 -I ~/.arduino15/packages/esp8266/hardware/esp8266/3.0.2/tools/espota.py -i <IPv4> -p 8266 --auth=P1P2MQTT -f P1P2-bridge-esp8266-0.9.24-Eseries-4MB.ino.bin
```
or
```
~/.arduino15/packages/esp8266/tools/python3/3.7.2-post1/python3 -I ~/.arduino15/packages/esp8266/hardware/esp8266/3.0.2/tools/espota.py -i <IPv4> -p 8266 --auth=P1P2MQTT -f P1P2-bridge-esp8266-0.9.24-Fseries-4MB.ino.bin
```

To install over USB with ESP01 programmer (Linux CLI):

```
~/.arduino15/packages/esp8266/tools/python3/3.7.2-post1/python3 -I ~/.arduino15/packages/esp8266/hardware/esp8266/3.0.2/tools/upload.py --chip esp8266 --port /dev/ttyUSB0 --baud 115200 --before default_reset --after hard_reset write_flash 0x0 P1P2-bridge-esp8266-0.9.24-Eseries-4MB.ino.bin
```
or
```
~/.arduino15/packages/esp8266/tools/python3/3.7.2-post1/python3 -I ~/.arduino15/packages/esp8266/hardware/esp8266/3.0.2/tools/upload.py --chip esp8266 --port /dev/ttyUSB0 --baud 115200 --before default_reset --after hard_reset write_flash 0x0 P1P2-bridge-esp8266-0.9.24-Eseries-4MB.ino.bin
```

To install OTA from Windows with espota.py from [here](https://github.com/esp8266/Arduino.git):

```
yourpath\Arduino-master\tools\espota.py -i <IPv4> -p 8266 --auth=P1P2MQTT -f P1P2-bridge-esp8266-0.9.24-Eseries-4MB.ino.bin
```

```
yourpath\Arduino-master\tools\espota.py -i <IPv4> -p 8266 --auth=P1P2MQTT -f P1P2-bridge-esp8266-0.9.24-Fseries-4MB.ino.bin
```

## FOSS notice

These images are built with Arduino BSPs and libraries, for which source code and license information is made available [here](../OSS-dependencies/README.md).
