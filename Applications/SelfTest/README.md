# Self-Test

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

With a serial port setup for serial bootloading (see BOOT_PORT in Makefile) and optiboot installed on the RPUno run 'make' and and 'make bootload' to upload a binary image in the application area of flash memory.


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
picocom -b 38400 /dev/ttyAMA0
...
Gravimetric Self Test date: Aug  9 2019
avr-gcc --version: 5.4.0
I2C provided address 0x31 from RPUadpt serial bus manager
adc reading for PWR_V: 359 int
PWR at: 12.740 V
ADC0 at ICP3&4 TERM /W all CS off: 0.000 V
ADC1 at ICP1 TERM /w all CS off: 0.000 V
ADC2 at ICP3&4 TERM /W all CS off: 0.000 V
ADC3 at ICP3&4 TERM /W all CS off: 0.000 V
ICP1 input should be HIGH with 0mA loop current: 1
CS_ICP1 on ICP1 TERM: 0.018 A
ICP1 /w 17mA on termination reads: 0
ICP3 input should be HIGH with 0mA loop current: 1
CS_ICP3 on ICP3AND4 TERM: 0.018 A
ICP3 /w 8mA on termination reads: 0
   ADC0 reading used to calculate ref_intern_1v1_uV: 857 int
   calculated ref_intern_1v1_uV: 1087701 uV
REF_EXTERN_AVCC old value found in eeprom: 4958300 uV
REF_INTERN_1V1 old value found in eeprom: 1085884 uV
REF_EXTERN_AVCC from eeprom is same
ICP4 input should be HIGH with 0mA loop current: 1
CS_ICP4 on ICP3AND4 TERM: 0.018 A
ICP4 /w 8mA on termination reads: 0
PWR_I at no load use INTERNAL_1V1: 0.019 A
CS0 on ICP3&4 TERM: 0.021 A
CS1 on ICP3&4 TERM: 0.021 A
CS2 on ICP3&4 TERM: 0.022 A
CS3 on ICP3&4 TERM: 0.022 A
TX1 loopback to RX1 == HIGH
TX1 loopback to RX1 == LOW
TX2 loopback to RX2 == HIGH
TX2 loopback to RX2 == LOW
SMBUS cmd 0 provided address 49 from manager
[PASS]
```

