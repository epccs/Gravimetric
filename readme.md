# Gravimetric

From <https://github.com/epccs/Gravimetric/>

## Overview

Board with ATmega324pb plumed for measuring event times using ICP1, ICP3, and ICP4 with headers for multi-drop serial Shields. 

The main idea guiding this design is gravimetric calibration of flow measuring devices. ICP1 is for measuring time events of flow pulses. ICP3 is for a start event and ICP4 a stop event, they have one-shot pulse extenders.  Flow diversion control is available that starts when the ICP3 capture event occurs and ends when the ICP4 capture event does. The ICP3 capture ISR needs to enable CS_DIVERSION and the ICP4 capture ISR disable CS_DIVERSION  to make the diversion control work properly. Serial input (or bit-bang for HX711) is available from RX1 and TX1 to connect a load cell amplifier. I2C is available to interface high-resolution ADC.

This board has a header for a Raspberry Pi SBC that can host the programming tools for the ATmega324pb controller (and the SBC is a computer if you need that sort of thing). The user can develop and upload applications from the SBC over the serial connection (which works like an [RPUpi]). 

[RPUpi]: https://github.com/epccs/RPUpi/

[Forum](https://rpubus.org/bb/viewforum.php?f=21)

## Status

![Status](./Hardware/status_icon.png "Status")

## [Hardware](./Hardware)

Hardware files include a schematic, bill of materials, and various notes for testing and evaluation. [Layout] files are seperate.

[Layout]: https://github.com/epccs/Eagle/


## Example

This boad has a serial bus that allows multiple boards to be connected to a Single Board Computer (SBC). The 40 pin header is for a Raspberry Pi but may work with other SBC's. I use a Pi Zero (and Zero W which has WiFi). The RJ45 connectors are for the multi-drop serial bus (RPUBUS) and allow the SBC to access other boards. This board has the [RPUpi] hardware on it so shield headers are removed. 

[RPUpi]: https://github.com/epccs/RPUpi/

![MultiDrop](./Hardware/Documents/MultiDrop.png "RPUicp MultiDrop")

Diverting a calibration fluid onto a scale during a precisely measured time while measuring the meter flow pulses is how I am going to calibrate my meters. The start and stop events will be synchronized to the diversion's control and their event time can be compared to the flow meter events. The start and stop can be from a volume that is being calibrated.

Note that the RPUBUS has been used and shown enough that I think it is time to simplified down to a single line.


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
