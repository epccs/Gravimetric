# Gravimetric

From <https://github.com/epccs/Gravimetric/>

## Overview

Board with ATmega324pb plumed for measuring event times using ICP1, ICP3, and ICP4 with headers for multi-drop serial Shields. ICP3 and ICP4 have one-shot pulse extenders. 

This programmable ATmega324pb based board has headers for a [RPUpi] or [RPUadpt] mezzanine shield. User's firmware application can monitor the power used with a high side current sense (ZXCT1087) and a voltage divider. Flow diversion control is available that starts when ICP3 capture occurs and ends when ICP4 capture does. The ICP3 capture ISR needs to enable a pull-up and the ICP4 capture ISR disable the pull-up to make the diversion control work properly. Serial input (can be a bit bang input for HX711) is available on the board to connect a load cell amplifier.

[RPUpi]: https://github.com/epccs/RPUpi/
[RPUadpt]: https://github.com/epccs/RPUadpt/


## Status

![Status](./Hardware/status_icon.png "Status")

## [Hardware](./Hardware)

Hardware files include a schematic, bill of materials, and various notes for testing and evaluation. [Layout] files are seperate.

[Layout]: https://github.com/epccs/Eagle/


## Example

This example shows an serial bus that allows multiple microcontroller boards to be connected to a single host computer serial port. It has an [RPUpi] shield that allows a Raspberry Pi Zero's hardware UART to connect as the host. The Pi Zero W has on board WiFi which I use for SSH connections and file sharing. The other controller boards use an [RPUadpt] shield and are daisy-chain with CAT5 cables. 

![MultiDrop](./Hardware/Documents/MultiDrop.png "RPUicp MultiDrop")

In the above setup... welp that drawing needs updates.


## AVR toolchain

This board uses the AVR toolchain, it is on Debian (e.g. Ubuntu and Raspbian), Windows, Mac and other Operating Systems. With the toolchain installed, the AVR application can compile locally (e.g. on a Raspberry Pi, Mac, or Windows 10 LSW). Make sure you understand that the hardware provider is not supplying the development tools and if they are broken then fixing them is a community effort. Also, it is worth noting that if the hardware provider changes its business model the tools will remain available from the community.

The core files for this board are in the /lib folder. Each example application has its files and a Makefile in a seperat folder. The toolchain is available as standard packages on Ubuntu and Raspbian. 

```
sudo apt-get install make git gcc-avr binutils-avr gdb-avr avr-libc avrdude
git clone https://github.com/epccs/Gravimetric
```

* [gcc-avr](https://packages.ubuntu.com/search?keywords=gcc-avr)
* [binutils-avr](https://packages.ubuntu.com/search?keywords=binutils-avr)
* [gdb-avr](https://packages.ubuntu.com/search?keywords=gdb-avr)
* [avr-libc](https://packages.ubuntu.com/search?keywords=avr-libc)
* [avrdude](https://packages.ubuntu.com/search?keywords=avrdude)

The software is a guide, it is in C because that works for me.
