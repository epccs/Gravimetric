# I2C1-Debug 

## Overview

2-wire Serial Interface (TWI, a.k.a. I2C) uses pins with SDA1 and SCL1 functions. 

The peripheral control software twi0_bsd.c depends on avr-libc and avr-gcc. It provides interrupt-driven asynchronous I2C functions that can operate without blocking. It can also work with an interleaving slave receive buffer for SMBus block transactions that appear to eliminate problems involving clock stretching (e.g., for R-Pi Zero). This application uses the I2C0 hardeare.

On Gravimetric, I2C1 is connected to the user access port.

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
cd /Gravimetric/Applications/i2c1-debug
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
{"id":{"name":"I2C1debug^2","desc":"Gravimetric (17341^1) Board /w ATmega324pb","avr-gcc":"5.4.0"}}
```

## /0/iscan?

Scan of I2C bus shows all 7 bit devices found (nothing is on the bus).

``` 
/1/iscan?
{"scan":[]}
```

Note: the address is right-shifted so that it does not include the Read/Write bit. 


## /0/iaddr 0..127

Set an I2C address for the masster to access. 

``` 
/1/iaddr 41
{"master_address":"0x29"}
```

Note: Set it with the decimal value, it will return the hex value. The address is used to write or read and will clear the transmit Buffer.


## /0/ibuff 0..255\[,0..255\[,0..255\[,0..255\[,0..255\]\]\]\]

Add up to five bytes to I2C transmit buffer. JSON reply is the full buffer. 

``` 
/1/ibuff 2,0
{"txBuffer[2]":[{"data":"0x2"},{"data":"0x0"}]}
``` 


## /0/ibuff?

Show transmit buffer data that will be given to I2C master.

``` 
/1/ibuff?
{"txBuffer[2]":[{"data":"0x2"},{"data":"0x0"}]}
``` 

## /0/iwrite

Attempt to become master and write the txBuffer bytes to I2C address. The txBuffer will clear if write was a success.

``` 
/1/iaddr 41
{"master_address":"0x29"}
/1/ibuff?
{"txBuffer[0]":[]}
/1/ibuff 2,0
{"txBuffer[2]":[{"data":"0x2"},{"data":"0x0"}]}
/1/iwrite
{"error":"wrt_addr_nack"}
``` 

Welp what did you expect? Nothing is on the bus.

## /0/iread? \[1..32\]

If txBuffer is empty, attempt to become master write zero bytes to chekc for NACK and then obtain readings into rxBuffer.

``` 
/1/iaddr 41
{"master_address":"0x29"}
/1/ibuff?
{"txBuffer[0]":[]}
/1/ibuff 2,0
{"txBuffer[2]":[{"data":"0x2"},{"data":"0x0"}]}
/1/iwrite
{"error":"wrt_addr_nack"}
/1/ibuff?
{"txBuffer[0]":[]}
/1/iread? 2
{"error":"wrt_addr_nack"}
``` 

If txBuffer has values, attempt to become master and write the byte(s) in buffer (e.g., a command byte) to I2C address without a stop condition. The txBuffer will clear if write was a success. Then send a repeated Start condition, followed by address and obtain readings into rxBuffer.

``` 
/1/iaddr 41
{"master_address":"0x29"}
/1/ibuff 2,0
{"txBuffer[2]":[{"data":"0x2"},{"data":"0x0"}]}
/1/iread? 2
{"error":"wrt_addr_nack"}
``` 

This way of doing the repeated start allows testing SMBus block commands, which need a command byte sent before a repeated start and finally reading the data block.


## /0/imon? \[7..119\]

Monitor a slave address. The receive callback (from master write) fills a print buffer and a local buffer for an echo of the data back during a transmit callback (to master read). Loading the print buffer will be skipped when the UART is writing.

``` 
/1/imon? 28
{"monitor_0x1C":[{"data":"0x2"},{"data":"0x0"}]}
{"monitor_0x1C":[{"data":"0x0"},{"data":"0x0"}]}
``` 

The monitor events are from another board with i2c where I used a master to write to this slave.

```
/0/iscan?
{"scan":[{"addr":"0x1C"},{"addr":"0x29"}]}
/0/iaddr 28
{"address":"0x1C"}
/0/ibuff 2,0
{"txBuffer[2]":[{"data":"0x2"},{"data":"0x0"}]}
/0/iread? 2
{"rxBuffer":[{"data":"0x2"},{"data":"0x0"}]}
/0/ibuff 0,0
{"txBuffer[2]":[{"data":"0x0"},{"data":"0x0"}]}
/0/iread? 2
{"rxBuffer":[{"data":"0x0"},{"data":"0x0"}]}
``` 


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
