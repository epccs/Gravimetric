# Makefile based non-blocking Blink

## Overview

Demonstration of General I/O, e.g. Blink an LED. 

Also shows the UART core and how to redirect it to stdin and stdout, as well as some Python that sends an 'a' character to stop the LED from blinking. 

## Firmware Upload

With a serial port connection (see BOOTLOAD_PORT in Makefile) and optiboot installed on the RPUno run 'make bootload' and it should compile and then flash the MCU.

``` 
sudo apt-get install make git gcc-avr binutils-avr gdb-avr avr-libc avrdude
git clone https://github.com/epccs/Gravimetric/Applications/BlinkLED
cd /Gravimetric/Applications/BlinkLED
make
make bootload
...
avrdude done.  Thank you.
``` 

# Notes

This program is done with plane C and a Makefile; it demonstrates a simple way to write an AVR program that avoids the use of heap memory. 

https://github.com/epccs/Document/tree/master/HeapAndStack