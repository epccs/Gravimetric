# Gravimetric

From <https://github.com/epccs/Gravimetric/>

## Overview

This board has an application micro-controller (ATmega324pb) with hardware set up for measuring some event times using ICP1 (flow pulse), ICP3 (start), and ICP4 (stop). It has two serial (UART) channels for user application, and a third serial connected to the RPUbus. There are some current source outputs and some inputs that can measure ADC or do digital IO. 

The board is a general-purpose controller in most ways, but there are a few areas aimed at the gravimetric calibration of flow measurement instruments. ICP1 is for measuring time events from flow meters (e.g., pulses). ICP3 is for a start event and ICP4 a stop event; ICP3 and ICP4 have one-shot pulse extenders.  Diversion control is a specialized circuit on this board; it starts when the ICP3 capture event occurs and ends with the ICP4 capture event. The ICP3 capture ISR needs to enable CS_DIVERSION, and the ICP4 capture ISR should disable CS_DIVERSION  for the diversion control to work correctly. Two serial inputs are available to connect a load cell amplifier (e.g., bit-bang an HX711 or serial with a calibrated scale) and a volume prover's inputs (e.g., bit-bang launch and ready or is there a prover with serial). I2C is prepared to interface with a high-resolution ADC (but let's get the onboard stuff working first).

I have integrated the RPUpi shield and removed shield headers; it has a 20x2 header pinout for a Raspberry Pi SBC, which has I2C (SMBus) to the 238pb manager, SPI to the 324pb controller, and of course a host-side interface to the RPUbus. The programming tools for the controller fit on the Raspberry Pi Zero, and that is adequate for flow calibration or as a flow computer. Many other options are present, including the new Pi4 if the SBC requires more computational or networking ability.  


## Status

![Status](./Hardware/status_icon.png "Status")

## [Hardware](./Hardware)

Hardware files include a schematic, bill of materials, and various notes for testing and evaluation. [Layout] files are separate.

[Layout]: https://github.com/epccs/Eagle/

Licenses have mostly changed from LGPL to BSD, but some work remains.


## Example

Is this comparable to a PLC? Probably not, in general, a PLC should run a set of well tested binary instructions that are ideally impossible to crash, but that does not stop it from causing severe damage to what it controls. Executing binary instructions is different; the binary executable can misconfigure the electronics as well as all the bad things a PLC can. The upshot of using machine code is the application is less limited by what was built into the PLC firmware, but make no mistake the training wheels are off, so use this sort of hardware at your own risk.

UART zero on the application MCU is used for a multi-drop serial bus that links multiple units to the serial hardware on one or more Single Board Computers (SBC). The link does not have a network footprint but does allow firmware uploads and general-purpose serial communication. The 40 pin header is for a Raspberry Pi, but other SBC's also work (I do not test them). The RJ45 connectors are for the multi-drop serial bus (not ethernet) and allow the SBC to access the other boards. 

![MultiDrop](./Hardware/Documents/MultiDrop.png "Gravimetric MultiDrop")

Diverting a calibration fluid onto a scale during a precisely measured time while measuring the meter flow pulses is how I am going to calibrate my meters. The start and stop events will be synchronized to the diversion control, and their event time can be compared to the flow meter events. The START and STOP can be from a volume that is being calibrated.


## AVR toolchain

This board uses the AVR toolchain. I use the one from Microchip on Debian, Ubuntu,  Raspbian, Windows with WSL. With the toolchain installed, the AVR application can compile localy. 

The frequently used files for this board are in the /lib folder. Each example application has its files and a Makefile in a separate folder. The toolchain is also available as packages. 

```
sudo apt-get install make git gcc-avr binutils-avr gdb-avr avr-libc avrdude
git clone https://github.com/epccs/Gravimetric
```

* [gcc-avr](https://packages.ubuntu.com/search?keywords=gcc-avr)
* [binutils-avr](https://packages.ubuntu.com/search?keywords=binutils-avr)
* [gdb-avr](https://packages.ubuntu.com/search?keywords=gdb-avr)
* [avr-libc](https://packages.ubuntu.com/search?keywords=avr-libc)
* [avrdude](https://packages.ubuntu.com/search?keywords=avrdude)

The software is a guide; it is in C because that works for me. 
