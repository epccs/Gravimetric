# To Do

Give the CLI a backspac.

https://www.avrfreaks.net/comment/2901251#comment-2901251


# Parsing serial for textual commands and arguments

## Overview

Demonstration of command parsing with the redirected stdio functions (e.g. printf() and simular) from avr-libc. 

## Firmware Upload

The manager needs to be told that it has a localhost and the address to bootload. I do this with some commands on the SMBus interface from an R-Pi.

``` 
sudo apt-get install i2c-tools python3-smbus
sudo usermod -a -G i2c your_user_name
# logout for the change to take
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# what is my address
bus.write_i2c_block_data(42, 0, [255])
print("'"+chr(bus.read_i2c_block_data(42, 0, 2)[1])+"'" )
'2'
# I want to bootload address '1'
bus.write_i2c_block_data(42, 3, [ord('1')])
print("'"+chr(bus.read_i2c_block_data(42, 3, 2)[1])+"'" )
'1'
# clear the host lockout status bit so nRTS from my R-Pi on '2' will triger the bootloader on '1'
bus.write_i2c_block_data(42, 7, [0])
print(bus.read_i2c_block_data(42,7, 2))
[7, 0]
exit()
```

Now the serial port connection (see BOOTLOAD_PORT in Makefile) can reset the MCU and execute optiboot so that the 'make bootload' rule can upload a new binary image in the application area of flash memory.

``` 
sudo apt-get install make git picocom gcc-avr binutils-avr gdb-avr avr-libc avrdude
git clone https://github.com/epccs/Gravimetric/
cd /Gravimetric/Applications/Parsing
make
make bootload
...
avrdude done.  Thank you.
``` 

Now connect with picocom (or ilk).

``` 
#exit is C-a, C-x
picocom -b 38400 /dev/ttyAMA0
#picocom -b 38400 /dev/ttyUSB0
``` 


# Command Line

/addr/command [arg[,arg[,arg[,arg[,arg]]]]]

The command is structured like a topic after the address but has optional arguments. An echo starts after the address byte is seen on the serial interface at 38400 baud rate. The UART core has a 32-byte buffer, which is optimal for AVR. Commands and arguments are ignored if they overflow the buffer.


## addr

The address is a single character. That is to say, "/0/id?" is at address '0', where "/id?" is a command. Echo of the command occurs after the address is received so the serial data can be checked as well as being somewhat keyboard friendly device.

## command

The command is made of one or more characters that are: alpha (see isalpha() function), '/', or '?'. 

## arg

The argument(s) are made of one or more characters that are: alphanumeric (see isalnum() function). 
