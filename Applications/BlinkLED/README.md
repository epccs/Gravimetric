# Makefile based non-blocking Blink

## Overview

Demonstration of General I/O, e.g. Blink an LED. 

Also shows the UART core and how to redirect it to stdin and stdout, as well as some Python that sends an 'a' character to stop the LED from blinking. 

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
cd /Gravimetric/Applications/BlinkLED
make
make bootload
...
avrdude done.  Thank you.
picocom -b 38400 /dev/ttyAMA0
...
a
#exit is C-a, C-x
``` 

# Notes

I want to thank the open-source community for toolchains and development environments that are challenging without being impossible. This program is done with plane C and a Makefile; it demonstrates a practical way to write an AVR program that avoids the madness of using shared heap and stack memory on this device.

https://github.com/epccs/Document/tree/master/HeapAndStack

