# Atmel Packs (Atmel.ATmega_DFP.1.4.346.atpack)

downloaded from http://packs.download.atmel.com/

Note: I have remove everything but the 328pb support for this project

# Usage

avr-gcc -mmcu=atmega324pb -B ../ATmega_DFP/gcc/dev/atmega324pb/ -I ../ATmega_DFP/include/

# Atmel toolchain

http://distribute.atmel.no/tools/opensource/Atmel-AVR-GNU-Toolchain/

https://www.microchip.com/mplab/avr-support/avr-and-sam-downloads-archive

I am just using avr-gcc packaged on Debian bullseye (e.g. 5.4.0+Atmel3.6.1-2 and others) it is on Ubuntu 2004.

https://packages.debian.org/bullseye/gcc-avr