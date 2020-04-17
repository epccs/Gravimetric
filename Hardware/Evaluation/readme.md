# Description

This shows the setup and method used for evaluation of Gravimetric.

# Table of References


# Table Of Contents:

1. ^1 I2C Multi-Master 
1. ^1 SMBus Block Read/Write Checked With Raspberry Pi Zero
1. ^1 SPI 2MHz Checked With Raspberry Pi Zero
1. ^1 UART with nRTS
1. ^1 UART sneaky mode
1. ^0 Start One Shot
1. ^0 Stop One Shot
1. ^0 Alternat Power Modified
1. ^0 Bootload
1. ^0 Bootloader and Manager fw
1. ^0 Mockup


## ^1 I2C Multi-Master

I2C has been overhauled at this time. The master now has non-blocking functions, and that can help open deadlocks. Some programs (i2c-debug and adc) are currently using a static loop state enumeration to allow software execution to return to the main loop while waiting for the TWI hardware. 

These changes opened the door to a multi-master operation.  The first implementation of which is the day-night state machine. To use it the application registers four values (bytes) with the manager first is a callback slave address. The next three bytes are the functions that deliver events. The events are day-night state machine changes, the start of the day, and the start of the night.

Only time will tell how well this will work, but I am pleased to remove polling, the application now feels as responsive as it did when the day-night state machine was on the application controller. 


## ^1 SMBus Block Read/Write Checked With Raspberry Pi Zero

Scan the SMBus (I2C1) with Raspberry Pi.

```
sudo apt-get install i2c-tools python3-smbus
# are my in the i2c group
[sudo apt install members]
members i2c
# no I am not
sudo usermod -a -G i2c rsutherland
# logout for the change to take
i2cdetect 1
WARNING! This program can confuse your I2C bus, cause data loss and worse!
I will probe file /dev/i2c-1.
I will probe address range 0x03-0x77.
Continue? [Y/n] Y
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- 2a -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- --
```

The manager firmware has the host locked out from using the serial connection (sneaky mode will bypass), but with the SMBus it can be enabled. One of the status bits is for host lockout. The SMBus command 7 is used to set that status bit.


``` 
python3
import smbus
bus = smbus.SMBus(1)
# use SMBus command 0 to show the local address 
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
bus.write_i2c_block_data(42, 0, [0])
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
#I2C_COMMAND must match in this case
chr(bus.read_i2c_block_data(42,0, 2)[1])
'1'
# use SMBus command 2 to show the bootload address 
bus.write_i2c_block_data(42, 2, [0])
chr(bus.read_i2c_block_data(42,2, 2)[1])
'0'
# use SMBus command 3 to change the bootload address (49 is ascii '1')
bus.write_i2c_block_data(42, 3, [49])
chr(bus.read_i2c_block_data(42,3, 2)[1])
'1'
# use SMBus command 7 to clear the lockout bit in the status bits that was set at power up.
bus.write_i2c_block_data(42, 7, [0])
print(bus.read_i2c_block_data(42,7, 2))
[7, 0]
exit()
picocom -b 38400 /dev/ttyAMA0
...
Terminal ready
/1/id?
# C-a, C-x.
``` 

Once the host lockout is clear the Raspberry Pi can bootload the address set with command 3 (see [Remote]). The Raspberry Pi will need to use its [RTS] handshack lines for avrdude to work.

[RTS]: https://github.com/epccs/RPUpi/tree/master/RPiRtsCts


## ^1 SPI 2MHz Checked With Raspberry Pi Zero

First bootload application controller with [SpiSlv].

[SpiSlv]:https://github.com/epccs/Gravimetric/tree/master/Applications/SpiSlv

To do that I need to clear the status lockout bit so the R-Pi Zero can use the multi-drop as a host (see "^1 UART with nRTS" bellow). 

``` 
# are my in the spi group
[sudo apt install members]
members spi
# no I am not
sudo usermod -a -G spi rsutherland
# logout for the change to take
# Next change the working directory to where SpiSlv is and then build and upload
cd Gravimetric/Applications/SpiSlv
make all
make bootload
...
avrdude done.  Thank you.

# 
gcc -o spidev_test spidev_test.c
chmod ugo+x ./spidev_test
# trun on the RPU SPI port
picocom -b 38400 /dev/ttyAMA0
...
Terminal ready
/1/id?
{"id":{"name":"SpiSlv","desc":"Gravimetric (17341^1) Board /w ATmega324pb","avr-gcc":"5.4.0"}}
/1/spi UP
{"SPI":"UP"}
# C-a, C-x.
# test with
./spidev_test -s 1000 -D /dev/spidev0.0
./spidev_test -s 10000 -D /dev/spidev0.0
./spidev_test -s 100000 -D /dev/spidev0.0
./spidev_test -s 250000 -D /dev/spidev0.0
./spidev_test -s 500000 -D /dev/spidev0.0
./spidev_test -s 1000000 -D /dev/spidev0.0
./spidev_test -s 2000000 -D /dev/spidev0.0
./spidev_test -s 4000000 -D /dev/spidev0.0
# 4MHz shows data on this Gravimetric^1 but 2MHz is max I would use.
```

The test output should look like this

```
spi mode: 0
bits per word: 8
max speed: 4000000 Hz (4000 KHz)

0D FF FF FF FF FF
FF 40 00 00 00 00
95 FF FF FF FF FF
FF FF FF FF FF FF
FF FF FF FF FF FF
FF DE AD BE EF BA
AD F0
``` 

The maximum speed is 2MegHz.


## ^1 UART with nRTS

Use sneaky mode to clear the status lockout bit so the R-Pi Zero can use the multi-drop as a host.

``` 
# Set '1' (0x31) as the bootload address that will be sent on DTR pair when nRTS enables.
# Next clear the lockout bit to alow the Pi Zero to use the multi-drop serial as a host.
picocom -b 38400 /dev/ttyAMA0
...
Terminal ready
/1/iaddr 41
{"master_address":"0x29"}
/1/ibuff 3,49
{"txBuffer[2]":[{"data":"0x3"},{"data":"0x31"}]}
/1/iread? 2
{"txBuffer":"wrt_success","rxBuffer":"rd_success","rxBuffer":[{"data":"0x3"},{"data":"0x31"}]}
/1/ibuff 7,0
{"txBuffer[2]":[{"data":"0x7"},{"data":"0x0"}]}
/1/iread? 2
# ASCII character glitch may show now since the local controller has just reset
# C-a, C-x.

# next check that the local manager which has multi-drop address '1' is blinking fast
picocom -b 38400 /dev/ttyAMA0
...
Terminal ready
# C-a, C-x.
```


## ^1 UART sneaky mode

Use picocom to connect to an application controller without enabling the host lockout status bit (see manager's I2C command 7).

``` 
# are my in the dialout group
[sudo apt install members]
members dialout
# no I am not
sudo usermod -a -G dialout rsutherland
# logout for the change to take
picocom -b 38400 /dev/ttyAMA0
...
Terminal ready
/1/id?
{"id":{"name":"I2C1debug^2","desc":"Gravimetric (17341^1) Board /w ATmega324pb","avr-gcc":"5.4.0"}}
``` 

The sneaky mode is a mistake that I use too much to fix. When state information is received on the DTR pair, the sneaky mode is done.


## ^0 Start One Shot

The pulse extender (one-shot) is evident in the image. The START input is toggled for 50uSec, and the One-Shot holds the ICP3 pin for about 1100 uSec. The CS_DIVERSION is sending 22mA to the ICP1 input resistor (100 Ohm, R119), and it is on during the One-Shot.

![StartOneShot](./17341^0_StartOneShot.jpg "Start One Shot")

Changing the time scale to 500 nSec/div we can see what the hardware signal timing looks like, the timing is fixed by hardware and is repeatable (e.g., this does not reduce measurement repeatability).

![StartOneShotFixed](./17341^0_StartOneShot_fixedOffset.jpg "Start One Shot Fixed Offset")

Reworked Q105 and Q118 was done to fix N-CH Cutoff issue noted in schooling for this.


## ^0 Stop One Shot

The pulse extender (one-shot) is evident in the image. The STOP input is toggled for 200uSec, and the One-Shot holds the ICP4 pin for about 1100 uSec. The CS_DIVERSION is sending 22mA to the ICP1 input resistor (100 Ohm, R119), and it is cut off during the One-Shot.

![StopOneShot](./17341^0_StopOneShot.jpg "Stop One Shot")

Changing the time scale to 500 nSec/div we can see what the hardware signal timing looks like, the timing is fixed by hardware and is repeatable (e.g., this does not reduce measurement repeatability).

![StopOneShotFixed](./17341^0_StopOneShot_fixedOffset.jpg "Stop One Shot Fixed Offset")

Reworked Q105 and Q118 was done to fix N-CH Cutoff issue noted in schooling for this.


## ^0 Alternat Power Modified

Alternate power has been modified so that D1 was replaced with a P-channel MOSFET with source facing Q2, and its gate tied to the gate of Q2. 

R1 used for ALT_V was cut and tied directly to ALT input. 

R4 was moved between the source of Q2 and the new P-channel MOSFET. 

Power protection was verified to work (e.g., backward polarity) while ALT_EN is off.

ALT_I checked. 

ALT_V checked. 

[DayNight] state machine, and [Alternat] power charging on a 12V battery checked.

[DayNight]: https://github.com/epccs/Gravimetric/tree/master/Applications/DayNight
[Alternat]: https://github.com/epccs/Gravimetric/tree/master/Applications/Alternat

These changes will show up on ^1, so they were hacked onto ^0 for evaluation.


## ^0 Bootload

Compile and bootload the BlinkLED firmware from an R-Pi Zero on a RPUpi^6 (on RPUno^9) over the RPUbus. 

The RPUpi^6 manager works with this manager at this time (other shields do not).

![Bootload](./17341^0_Bootload.jpg "Bootload")

As you can see, the R-Pi header and its POL are not used; it is remote or foreign relative to the host.


## ^0 Bootloader and Manager fw

I was not able to install with the ICSP and R-Pi Zero for reasons that are not yet clear, but the ArduinoISP sketch worked and the board firmware installed. I will backtrack to the ICSP as time allows, next is to verify the bootloader works with the BlinkLED fw which needs Uart and Timer drivers, then I need the ADC drivers for the self-test. 


## ^0 Mockup

Parts are not soldered, they are just resting in place, the headers are in separate BOM's and not included unless specified (they are expensive but in some cases worth the cost).

![MockupWithVertRPiZero](./17341^0_MockupWithVertRPiZero.jpg "Mockup With Vert R-Pi Zero")

With Horizontal Raspberry Pi Zero

![MockupWithHorzRPiZero](./17341^0_MockupWithHorzRPiZero.jpg "Mockup With Horz R-Pi Zero")

With Horizontal Raspberry Pi 3B+ (note the POL is missing)

![MockupWithRPi3B+](./17341^0_MockupWithRPi3B+_over.jpg "Mockup With R-Pi 3B+")

side view

![MockupWithRPi3B+_side](./17341^0_MockupWithRPi3B+_side.jpg "Mockup With R-Pi 3B+ Side")

The fit is surprisingly good. The edge of the ethernet connector is over the Gravimetric board just enough. The ethernet connector is resting on the plastic DIN rail clip. I suspect that an additional DIN clip could have some hook and loop (Velcro) placed on it and the top of the Ethernet header to fasten the board. One thing to note is that the screw head holding the Gravimetric board to the DIN rail clip needs some filing, so its edge does not go over the side of the Gravimetric board.

The computing power of a R-Pi 3 B+ is significant. It has a quad core Cortex-A53 (ARMv8) 64-bit SoC running at 1.4GHz. Unfortunately it needs 2.5A at 5V, and I have yet to find an option I like for the POL, I want it to turn 7V thru 36V into 5V@3A.

The R-Pi has a lot of approval certificates now days, so it is a choice for areas that need that sort of stuff.

https://www.raspberrypi.org/documentation/hardware/raspberrypi/conformity.md
