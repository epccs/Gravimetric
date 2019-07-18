# Description

This is a list of Test preformed on each Gravimetric board after assembly.

# Table of References


# Table Of Contents:

1. Basics
2. Assembly check
3. IC Solder Test
4. Power Protection
5. Power Without SMPS
6. Bias +5V
7. Install Bootloader

## Basics

These tests are for an assembled Gravimetric board 17341^0 which may be referred to as a Unit Under Test (UUT). If the UUT fails and can be reworked then do so, otherwise it needs to be scraped. 

**Warning: never use a soldering iron to rework crystals or ceramic capacitors due to the thermal shock.**
    
Items used for test.

![ItemsUsedForTest](./17341,ItemsUsedForTest.jpg "Gravimetric Items Used For Test")


## Assembly check

After assembly check the circuit carefully to make sure all parts are soldered and correct, the device marking is labeled on the schematic and assembly drawing.

NOTE: U3 and U4 are not yet on the board, so +5V will not have power.


## IC Solder Test

U3 and U4 are not yet populated. Check that a diode drop to 0V is present from a circuit board pad that is connected to each of the pins of the integrated circuits, measure with a DMM's diode test set up for reversed polarity. Consult the schematic to determine which pins can be skipped (e.g. ground, power rail, ...).

This is a simplified In-Circuit Test (ICT). It could become more valuable if the node voltage driven with the current source was recorded for each tested location and then used with statistics to determine test limits for each location. 

## Power Protection

Apply a current limited (20mA) supply set with 5V to the PWR and 0V connector J8 in reverse and verify that the voltage does not get through. Adjust the supply to 36V and verify no current is passing.


## Power Without SMPS

Apply a current limited (20mA) supply set with 7V to the PWR and 0V connector J7 and verify that the voltage does get through. Adjust the supply so the LED is on and stable and measure voltage, adjust supply to 30V measure input current. 

NOTE for referance the zener voltage on Q5 is 7.75V at 30V.

```
{ "LEDON_V":[10.6,],
  "PWR@7V_mA":[0.125,],
  "PWR@30V_mA":[1.5,] }
```


## Bias +5V

Apply a 30mA current limited 5V source to +5V (J1). Check that the input current is for two blank MCU (e.g., manager and application). Turn off the power.

```
{ "I_IN_BLANKMCU_mA":[4.7,]}
```

Note: Internal clock/8 (=1MHz) and IO pins are floating (a test fixture is needed).


## Install Bootloader

Install Git and AVR toolchain on Ubuntu (18.04). 

```
sudo apt-get install make git gcc-avr binutils-avr gdb-avr avr-libc avrdude
```

Clone the RPUno repository.

```
cd ~
git clone https://github.com/epccs/RPUno
cd ~/RPUno/Bootloader
```

Connect a 5V supply with CC mode set at 30mA to +5V (J7). Connect the ISP tool (J11). The MCU needs its fuses set, so run the Makefile rule to do that. 

```
make fuse
```

Next install the bootloader

```
make isp
```

Disconnect the ICSP tool and measure the input current, wait for the power to be settled. Turn off the power.

```
{ "I_IN_16MHZ_EXT_CRYST_mA":[12.7,11.2,11.1,11.0,11.0,10.8,10.6,10.7,]}
```

Add U2 to the board now. Measurement of the input current is for referance (takes a long time to settle, 10mA ICP1 jumper is off).
