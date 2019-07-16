# Gravimetric

From <https://github.com/epccs/Gravimetric/>

## Overview

This board has an application micro-controller (ATmega324pb) with hardware set up for measuring some event times using ICP1 (flow pulse), ICP3 (start), and ICP4 (stop). It has two serial (UART) channels, and a third (main) serial connected to a muli-drop bus (RPUBUS). There are some current source outputs and some inputs that can measure ADC or do digital IO. 

The board is a general purpose controller in many ways, but there are a few areas aimed at the gravimetric calibration of flow measurement instruments. ICP1 is for measuring time events from flow meters (e.g., pulses). ICP3 is for a start event and ICP4 a stop event; ICP3 and ICP4 have one-shot pulse extenders.  Diversion control is a specialized circuit on this board; it starts when the ICP3 capture event occurs and ends when the ICP4 capture event begins. The ICP3 capture ISR needs to enable CS_DIVERSION, and the ICP4 capture ISR should disable CS_DIVERSION  for the diversion control to work correctly. Two serial inputs are available to connect a load cell amplifier (e.g., bit-bang an HX711 or serial with a calibrated scale) and a volume prover's inputs (e.g., bit-bang launch and ready or is there a prover with serial). I2C is prepared to interface with a high-resolution ADC (but let's get the onboard stuff working first).

I have ditched the shield headers on this board; it has a 20x2 header pinout for a Raspberry Pi SBC, which has I2C (SMBus) to the 238pb manager, SPI to the 324pb controller, and of course a host-side interface to the multi-drop serial bus. I like to have the programming tools for the controller with the SBC at the controller, but it is also a general purpose networking computer that can interface with the controller over its hard links (e.g., what I call non-network connections like SPI, I2C, UART). A serial bootloader on the 324pb allows an upload tool (avrdude) to send applications compiled on the SBC to run on the small dedicated machine.


## Status

![Status](./Hardware/status_icon.png "Status")

## [Hardware](./Hardware)

Hardware files include a schematic, bill of materials, and various notes for testing and evaluation. [Layout] files are separate.

[Layout]: https://github.com/epccs/Eagle/


## Example

Is this comparable to a PLC? Probably not, in general, it should be impossible to cause a PLC to crash its software, but that does not stop it from causing severe damage. Executing binary files is a whole different game; the program can both crash and do severe damage. The upshot is the application is only limited by what the microcontroller can do (the training wheels are off, so use this at your own risk).

This board has a serial bus that allows multiple boards to be connected to a Single Board Computer (SBC). The 40 pin header is for a Raspberry Pi but other SBC's also work (I do not test them). I use a Pi Zero (and Zero W which has WiFi). The RJ45 connectors are for the multi-drop serial bus (not ethernet) and allow the SBC to access the other boards. 

![MultiDrop](./Hardware/Documents/MultiDrop.png "Gravimetric MultiDrop")

Diverting a calibration fluid onto a scale during a precisely measured time while measuring the meter flow pulses is how I am going to calibrate my meters. The start and stop events will be synchronized to the diversion's control, and their event time can be compared to the flow meter events. The START and STOP can be from a volume that is being calibrated.


## AVR toolchain

This board uses the AVR toolchain; it is on Debian (e.g., Ubuntu and Raspbian), Windows, Mac and other Operating Systems. With the toolchain installed, the AVR application can compile locally (e.g., on a Raspberry Pi, Mac, or Windows 10 LSW). Make sure you understand that the hardware provider is not supplying the development tools, and if they are broken, then fixing them is a community effort. Also, it is worth noting that if the hardware provider changes its business model, the tools will remain available from the community.

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

Warning: for the 324pb Microchip has changed how AVR defines stuff (name of bits in registers mostly), so expect pain when updating libraries. 
