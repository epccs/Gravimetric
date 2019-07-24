# I2C0-Debug 

## Overview

2-wire Serial Interface (TWI, a.k.a. I2C) uses pins with SDA0 and SCL0 functions. 

The driver twi0.c and twi0.h depends on avr-libc and avr-gcc. It uses an ASYNC ISR but does block (busy wait) while reading or writing during which it will do clock stretching. 

Since this application only uses the I2C0 as a master clock stretching does not matter, but if an application needs an I2C slave (e.g., on twi1), then an interleaving buffer is desirable to eliminate clock stretching with which some masters struggle (e.g., R-Pi Zero).

Note: On Gravimetric I2C0 does not have user-accessible pins, it is only connected to the manager at this time. I2C1 is connected to the user access port and may be used as a slave or a master.

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
cd /Gravimetric/Applications/i2c0-debug
make
make bootload
...
avrdude
``` 

Now connect with picocom (or ilk).


``` 
#exit is C-a, C-x
picocom -b 38400 /dev/ttyAMA0
``` 


# Commands

Commands are interactive over the serial interface at 38400 baud rate. The echo will start after the second character of a new line. 


## /\[rpu_address\]/\[command \[arg\]\]

rpu_address is taken from the manager (e.g. get_Rpu_address() which is included form ../lib/rpu_mgr.h). The value of rpu_address is used as a character in a string, which means don't use a null value (C strings are null terminated) as an adddress. The ASCII value for '1' (0x31) is easy and looks nice, though I fear it will cause some confusion when it is discovered that the actual address value is 49.

Commands and their arguments follow.

## /0/id? \[name|desc|avr-gcc\]

identify 

``` 
/1/id?
{"id":{"name":"I2C0debug^1","desc":"Gravimetric (17341^0) Board /w ATmega324pb","avr-gcc":"5.4.0"}}
```

## /0/iscan?

Scan of I2C bus shows all 7 bit devices found. I have a PCA9554 at 0x38 and an 24C02AN eeprom at 0x50.

``` 
/1/iscan?
{"scan":[{"addr":"0x29"},{"addr":"0x38"},{"addr":"0x50"}]}
```

Note: the address does not include the Read/Write bit. 


## /0/iaddr 0..127

Set the I2C address 

``` 
/1/iaddr 56
{"address":"0x38"}
```

Note: Set it with the decimel value, it will return the hex value. This value is used durring read and write, it will also reset the xtBuffer.


## /0/ibuff 0..255\[,0..255\[,0..255\[,0..255\[,0..255\]\]\]\]

Add up to five bytes to I2C transmit buffer. JSON reply is the full buffer. 

``` 
/1/ibuff 3,0
{"txBuffer":["data":"0x3","data":"0x0"]}
``` 


## /0/ibuff?

Show buffer data.

``` 
/1/ibuff?
{"txBuffer":["data":"0x3","data":"0x0"]}
``` 

## /0/iwrite

Attempt to become master and write the txBuffer bytes to I2C address (PCA9554). The txBuffer will clear if write was a success.

``` 
/1/iaddr 56
{"address":"0x38"}
/1/ibuff 3,0
{"txBuffer":["data":"0x3","data":"0x0"]}
/1/iwrite
{"returnCode":"success"}
``` 

## /0/iread? \[1..32\]

If txBuffer has values, attempt to become master and write the byte(s) in buffer (e.g. command byte) to I2C address (example is for a PCA9554) without a stop condition. The txBuffer will clear if write was a success. Then send a repeated Start condition, followed by address and obtain readings into rxBuffer.

``` 
/1/iaddr 56
{"address":"0x38"}
/1/ibuff 3
{"txBuffer":["data":"0x3"}
/1/iread? 1
{"rxBuffer":[{"data":"0xFF"}]}
``` 

Note the PCA9554 has been power cycled in this example, so the reading is the default from register 3.


# PCA9554 Example

Load the PCA9554 configuration register 3 (DDR) with zero to set the port as output. Then alternate register 1 (the output port) with 85 and 170 to toggle its output pins. 

``` 
/1/iaddr 56
{"address":"0x38"}
/1/ibuff 3,0
{"txBuffer":["data":"0x3","data":"0x0"]}
/1/iwrite
{"returnCode":"success"}
/1/ibuff 1,170
{"txBuffer":[{"data":"0x1"},{"data":"0xAA"}]}
/1/iwrite
{"returnCode":"success"}
/1/ibuff 1,85
{"txBuffer":[{"data":"0x1"},{"data":"0x55"}]}
/1/iwrite
{"returnCode":"success"}
``` 

# HTU21D Example 

``` 
/1/scan?
{"scan":[{"addr":"0x29"},{"addr":"0x40"}]}
``` 

Command 0xE3 measures temperature, the clock is streached until data is ready.

``` 
/1/iaddr 64
{"address":"0x40"}
/1/ibuff 227
{"txBuffer":["data":"0xE3"]}
/1/iread? 3
{"rxBuffer":[{"data":"0x6A"},{"data":"0xC"},{"data":"0xC6"}]}
``` 

The first two bytes are the temperature data. The last two bits of LSB are status (ignore or mask them off). Some Python gives the result in deg C.

``` 
Stmp = 0x6A0C
Temp = -46.85 + 175.72 * Stmp / (2**16)
Temp
25.9
``` 

Command 0xE5 measures humidity, again the clock is streached until data is ready.

``` 
/1/iaddr 64
{"address":"0x40"}
/1/ibuff 229
{"txBuffer":["data":"0xE5"]}
/1/read? 3
{"rxBuffer":[{"data":"0x65"},{"data":"0x96"},{"data":"0xBC"}]}
``` 

The first two bytes are the temperature data. The last two bits of LSB are status (ignore or mask them off). Some Python gives the result in deg C.

``` 
Stmp = 0x6596 & 0xFFFC
RH = -6 + 125 * Stmp / (2**16)
RH
43.6
``` 
