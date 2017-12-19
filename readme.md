# RPUicp

From <https://github.com/epccs/RPUicp/>

## Overview

Board with ATmega328pb plumed for measuring event times using ICP1, ICP3, and ICP4 with headers for multi-drop serial Shields

This programmable ATmega328pb based board has headers for a [RPUpi] or [RPUadpt] mezzanine shield. User's firmware application can monitor the input current with a high side current sense (ZXCT1087) with ADC channel three, and the input voltage with a voltage divider on channel seven. Input capture is available on three timers from Timer1, Timer3, and Timer4.  

[RPUpi]: https://github.com/epccs/RPUpi/
[RPUadpt]: https://github.com/epccs/RPUadpt/


## Status

![Status](./Hardware/status_icon.png "Status")

## [Hardware](./Hardware)

Hardware files include a schematic, bill of materials, and various notes for testing and evaluation. [Layout] files are seperate.

[Layout]: https://github.com/epccs/Eagle/


## Example

This example shows an RS-422 serial bus that allows multiple microcontroller boards to be connected to a single host computer serial port. It has an [RPUpi] shield that allows the Raspberry Pi Zero's hardware UART to connect as the host. The Pi Zero W has on board WiFi which I use for SSH connections and Samba file sharing. The other controller boards use an [RPUadpt] shield to daisy-chain the RS-422 with CAT5 cables. 

![MultiDrop](./Hardware/Documents/MultiDrop.png "RPUicp MultiDrop")

In the above setup, the calibration meter has good correlation with the volume and should thus be a good calibration source for other meters. The calibration meter could run for longer times with meters that don't have a timing source that is shared with the test volume which could help to overcome the correlation uncertainty caused by using separate timing sources.


## AVR toolchain

This board uses the open source AVR toolchain found on Debian that has spread to most of the other Operating Systems. With the toolchain installed, the AVR application can compile locally (e.g. on a Raspberry Pi, Mac, or Windows 10 LSW). Make sure you understand that the hardware provider is not supplying the development tools and if they are broken then fixing them is a community effort. Also, it is worth noting that if the hardware provider changes its business model the tools will remain available from the community.

The core files for this board are in the /lib folder. Each example has its files and a Makefile in its own folder. The toolchain is available as standard packages on Ubuntu and Raspbian. 

```
sudo apt-get install git gcc-avr binutils-avr gdb-avr avr-libc avrdude
git clone https://github.com/epccs/RPUlux/
```

* [gcc-avr](http://packages.ubuntu.com/search?keywords=gcc-avr)
* [binutils-avr](http://packages.ubuntu.com/search?keywords=binutils-avr)
* [gdb-avr](http://packages.ubuntu.com/search?keywords=gdb-avr)
* [avr-libc](http://packages.ubuntu.com/search?keywords=avr-libc)
* [avrdude](http://packages.ubuntu.com/search?keywords=avrdude)

The software is a guide, it is in C because that works for me.
