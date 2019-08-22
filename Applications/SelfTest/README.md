# Self-Test

## known issue (WIP

Added Start one-shot test. When ICP3 goes low it will enable the Diversion CS which self-test wires to the ICP1 input so check that it is LOW and take record of millis(), then loop until ICP1 is HIGH and report one-shot time. Finaly enable CS_DIVERSION. 

Added Stop one-shot test. When IPC4 goes low it will disable CS_DIVERSION. The ICP1 input should go HIGH and a millis() record taken, then loop until ICP1 goes LOW again and report ICP4 one-shot time. Finaly disable CS_DIVERSION.

Added test for CS_DIVERSION, CS_FAST, and CS4 that sends current to the ICP1 input.

Problem CS_DIVERSION is running at 50mA, why is TBD. Other errors are due to R-Pi header not having +5V.

Use bootload port to view test. Remote fw on manager needs more work to recover after test mode.


## Overview

Check Board Functions, runs once after a reset and then loops in a pass/fail section.

ICP1, ICP3, and ICP4 each have a 100 Ohm resistor to 0V on board.

Voltage references are saved in EEPROM for use with Adc and other applications. Measure the +5V supply accurately and set the REF_EXTERN_AVCC value in the main.c file. The band-gap reference is calculated and also saved.

The wiring has a red and green LED that blink to indicate test status.

## Wiring Needed for Testing

![Wiring](./Setup/SelfTestWiring.png)

not yet shown:
I2C1 on 324pb wired to SMBus on R-Pi header
+5V from bootload port pin wired to +5V on R-Pi header
all loopbacks from [XcvrTest] on R-Pi header

[XcvrTest]: https://github.com/epccs/RPUno/tree/master/XcvrTest


## Power Supply

Use a power supply with CV and CC mode. Set CC at 200mA and set CV to 12.8V then connect and power the UUT.


## Firmware Upload

With a serial port setup for serial bootloading (see BOOT_PORT in Makefile) and optiboot installed on the DUT run 'make' and and 'make bootload' to upload a binary image in the application area of flash memory.


``` 
sudo apt-get install make git picocom gcc-avr binutils-avr gdb-avr avr-libc avrdude
git clone https://github.com/epccs/Gravimetric/
cd /Gravimetric/Applications/SelfTest
make
make bootload
...
avrdude done.  Thank you.
make clean
``` 

Now connect with picocom (exit is C-a, C-x). 

``` 
#picocom -b 38400 /dev/ttyAMA0
# use bootload port
picocom -b 38400 /dev/ttyUSB0
...
Gravimetric Self Test date: Aug 21 2019
avr-gcc --version: 5.4.0
I2C provided address 0x31 from RPUadpt serial bus manager
adc reading for PWR_V: 360 int
PWR at: 12.776 V
ADC0 at ICP3&4 TERM /W all CS off: 0.000 V
ADC1 at ICP1 TERM /w all CS off: 0.000 V
ADC2 at ICP3&4 TERM /W all CS off: 0.000 V
ADC3 at ICP3&4 TERM /W all CS off: 0.000 V
ICP1 input should be HIGH with 0mA loop current: 1
CS_ICP1 on ICP1 TERM: 0.018 A
ICP1 /w 17mA on termination reads: 0
CS4 on ICP1 TERM: 0.022 A
CS_FAST on ICP1 TERM: 0.021 A
ICP3 input should be HIGH with 0mA loop current: 1
ICP3 one-shot delay: 0 mSec
ICP3 one-shot time: 1 mSec
CS_ICP3 on ICP3AND4 TERM: 0.018 A
ICP3 /w 8mA on termination reads: 0
   ADC0 reading used to calculate ref_intern_1v1_uV: 857 int
   calculated ref_intern_1v1_uV: 1087701 uV
REF_EXTERN_AVCC old value found in eeprom: 4958300 uV
REF_INTERN_1V1 old value found in eeprom: 1085884 uV
REF_EXTERN_AVCC from eeprom is same
ICP4 input should be HIGH with 0mA loop current: 1
CS_DIVERSION on ICP1 TERM: 0.050 A
>>> CS_DIVERSION curr is to high.
>>> ICP4 CS_DIVERSION did not end befor timeout: 101
>>> ICP4 one-shot ends CS_DIVERSION but that was not seen on ICP1.
CS_ICP4 on ICP3AND4 TERM: 0.018 A
ICP4 /w 8mA on termination reads: 0
PWR_I at no load use INTERNAL_1V1: 0.022 A
CS0 on ICP3&4 TERM: 0.021 A
CS1 on ICP3&4 TERM: 0.021 A
CS2 on ICP3&4 TERM: 0.022 A
CS3 on ICP3&4 TERM: 0.022 A
TX1 loopback to RX1 == HIGH
TX1 loopback to RX1 == LOW
TX2 loopback to RX2 == HIGH
TX2 loopback to RX2 == LOW
SMBUS cmd 0 provided address 49 from manager
MISO loopback to MOSI == HIGH
>>> MISO 0 did not loopback to MOSI 1
>>> R-Pi POL pin needs 5V for loopback to work
SCK with Shutdown loopback == HIGH
I2C0 Shutdown cmd is clean {5, 1}
>>> Shutdown LOW did not loopback to SCK 1
I2C0 Shutdown Detect cmd is clean {4, 1}

Testmode: default trancever control bits
I2C0 Start Test Mode cmd was clean {48, 1}
I2C0 End Test Mode hex is Xcvr cntl bits {49, 0xD5}
Testmode: read  Xcvr cntl bits {50, 0xE2}
PWR_I /w no load using INTERNAL_1V1: 0.008 A

Testmode: nCTS loopback to nRTS
I2C0 Start Test Mode cmd was clean {48, 1}
I2C0 End Test Mode hex is Xcvr cntl bits {49, 0xD5}
Testmode: set  Xcvr cntl bits {51, 0xA2}
>>> Xcvr cntl bits should be 22 but report was a2

Testmode: Enable TX pair driver
 I2C0 Start Test Mode cmd was clean {48, 1}
I2C0 End Test Mode hex is Xcvr cntl bits {49, 0xD5}
Testmode: set  Xcvr cntl bits {51, 0xF2}
Testmode: read  Xcvr cntl bits {50, 0xF2}
PWR_I /w TX pair load: 0.028 A

Testmode: Enable TX & RX(loopback) pair drivers
 I2C0 Start Test Mode cmd was clean {48, 1}
I2C0 End Test Mode hex is Xcvr cntl bits {49, 0xD5}
Testmode: set  Xcvr cntl bits {51, 0xD1}
Testmode: read  Xcvr cntl bits {50, 0xD1}
PWR_I /w TX and RX pairs loaded: 0.027 A
>>> TX and RX pairs load curr are too low.
>>> RX loopback should be LOW but was HIGH

Testmode: Enable DTR pair driver
I2C0 Start Test Mode cmd was clean {48, 1}
I2C0 End Test Mode hex is Xcvr cntl bits {49, 0xD5}
Testmode: set  Xcvr cntl bits {51, 0xE6}
Testmode: read  Xcvr cntl bits {50, 0xE6}
PWR_I /w DTR pair load: 0.027 A
[FAIL]
```

