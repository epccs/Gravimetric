# Atmel Packs (Atmel.ATmega_DFP.1.3.300.atpack)

downloaded from http://packs.download.atmel.com/

Note: I have remove everything but the 324pb support

# Usage

avr-gcc -mmcu=atmega324pb -B ../Atmel.ATmega_DFP.1.3.300.atpack/gcc/dev/atmega324pb/ -I ../Atmel.ATmega_DFP.1.3.300.atpack/include/

# Atmel toolchain

http://distribute.atmel.no/tools/opensource/Atmel-AVR-GNU-Toolchain/

I am just using avr-gcc packaged on Debian buster (e.g. 5.4.0 it is on Ubuntu 1804)
