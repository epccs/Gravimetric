# Test Mode Commands

48..63 (Ox30..0x3F | 0b00110000..0b00111111)

48. save trancever control bits HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE for test_mode.
49. recover trancever control bits after test_mode.
50. read trancever control bits durring test_mode, e.g. 0b11101010 is HOST_nRTS = 1, HOST_nCTS =1, DTR_nRE =1, TX_nRE = 1, TX_DE =0, DTR_nRE =1, DTR_DE = 0, RX_nRE =1, RX_DE = 0.
51. set trancever control bits durring test_mode, e.g. 0b11101010 is HOST_nRTS = 1, HOST_nCTS =1, TX_nRE = 1, TX_DE =0, DTR_nRE =1, DTR_DE = 0, RX_nRE =1, RX_DE = 0.


## Cmd 48 from a controller /w i2c-debug set transceiver test mode

Set test_mode, use the bootload port interface since the RPUbus transceivers are turned off.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 48,1
{"txBuffer[2]":[{"data":"0x30"},{"data":"0x1"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x30"},{"data":"0x1"}]}
``` 

on the second I2C channel that is for SMBus's i2c block commands

``` 
picocom -b 38400 /dev/ttyUSB0
/0/iaddr 42
{"address":"0x2A"}
/0/ibuff 48,1
{"txBuffer[2]":[{"data":"0x30"},{"data":"0x1"}]}
/0/iwrite
{"returnCode":"success"}
/0/ibuff 48
{"txBuffer[1]":[{"data":"0x30"}]}
/0/iread? 2
{"rxBuffer":[{"data":"0x30"},{"data":"0x1"}]}
```

The first read does a write. SMBus then does a second write with the command and then a repeated start followed by reading the data. I am using a old buffer to supply the data from the first write. The command byte needs to match to setup the old buffer data for use by the transmit event callback. 


## Cmd 48 from a Raspberry Pi set transceiver test mode

Set test_mode with an R-Pi over SMBus.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# what is my address
bus.write_i2c_block_data(42, 0, [255])
bus.read_i2c_block_data(42, 0, 2)
[0, 50]
# set the bootload address to my address
bus.write_i2c_block_data(42, 3, [50])
bus.read_i2c_block_data(42, 3, 2)
[3, 50]
# clear the host lockout status bit so serial from this host can work
bus.write_i2c_block_data(42, 7, [0])
print(bus.read_i2c_block_data(42,7, 2))
[7, 0]
exit()
# on an RPUno load the blinkLED application which uses the UART
git clone https://github.com/epccs/RPUno
cd /RPUno/BlinkLED
make
make bootload
# now back to 
python3
import smbus
bus = smbus.SMBus(1)
# and set the test_mode
bus.write_i2c_block_data(42, 48, [1])
print(bus.read_i2c_block_data(42, 48, 2))
[48, 1]
``` 

I had picocom in another SSH connection to see how the test mode was working (e.g., it turns off the transceivers and serial stops operating). 

``` 
picocom -b 38400 /dev/ttyAMA0
a

``` 

Use  command 50 to set the transceiver control bits and check them with picocom.


## Cmd 49 from a controller /w i2c-debug recover after transceiver test

Recover trancever control bits after test_mode.

data returned is the recoverd trancever control bits: HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 49,1
{"txBuffer[2]":[{"data":"0x31"},{"data":"0x1"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x31"},{"data":"0x15"}]}
``` 

on the second I2C channel that is for SMBus's i2c block commands

``` 
picocom -b 38400 /dev/ttyUSB0
/0/iaddr 42
{"address":"0x2A"}
/0/ibuff 49,1
{"txBuffer[2]":[{"data":"0x31"},{"data":"0x1"}]}
/0/iwrite
{"returnCode":"success"}
/0/ibuff 49
{"txBuffer[1]":[{"data":"0x31"}]}
/0/iread? 2
{"rxBuffer":[{"data":"0x31"},{"data":"0x15"}]}
``` 

The read has the old buffer from the write command, it show data that can be seen after power up e.g., 0x15 or 0b00010101. The trancever control bits are: HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE


## Cmd 49 from a Raspberry Pi recover after transceiver test

Set test_mode with an R-Pi over SMBus.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# end the test_mode data byte is replaced with the recoverd trancever control
bus.write_i2c_block_data(42, 49, [1])
print(bus.read_i2c_block_data(42, 49, 2))
[49, 21]
bin(21)
'0b10101'
``` 

The trancever control bits are: HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE

So everything was enabled, it is sort of a stealth mode after power-up, and can allow a host to talk to new controllers on the bus if they don't toggle there RTS lines (e.g., the test case is an R-Pi on an RPUpi shield on an RPUno, all freshly powered.) Stealth mode ends when a byte is seen on the DTR pair, that is what will establish an accurate bus state, so stealth mode is an artifact of laziness after power up and may need to change. 


## Cmd 50 from a controller /w i2c-debug read trancever control bits

Read trancever control bits (HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE) durring test_mode on bootload port with i2c-debug.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 50,255
{"txBuffer[2]":[{"data":"0x32"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x32"},{"data":"0x22"}]}
``` 

on the second I2C channel that is for SMBus's i2c block commands

``` 
picocom -b 38400 /dev/ttyUSB0
/0/iaddr 42
{"address":"0x2A"}
/0/ibuff 50,255
{"txBuffer[2]":[{"data":"0x32"},{"data":"0xFF"}]}
/0/iwrite
{"returnCode":"success"}
/0/ibuff 50
{"txBuffer[1]":[{"data":"0x32"}]}
/0/iread? 2
{"rxBuffer":[{"data":"0x32"},{"data":"0x22"}]}
```


## Cmd 50 from a Raspberry Pi read trancever control bits

Read trancever control bits (HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE) durring test_mode with an R-Pi over SMBus.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# end the test_mode data byte is replaced with the recoverd trancever control
bus.write_i2c_block_data(42, 50, [255])
print(bus.read_i2c_block_data(42, 50, 2))
[50, 34]
``` 


## Cmd 51 from a controller /w i2c-debug set trancever control bits

Set trancever control bits (HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE) durring test_mode on bootload port with i2c-debug.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 51,38
{"txBuffer[2]":[{"data":"0x33"},{"data":"0x26"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x33"},{"data":"0x26"}]}
``` 

0x26 = HOST_nRTS==0:HOST_nCTS=0:TX_nRE==1:TX_DE==0:DTR_nRE==0:DTR_DE==1:RX_nRE==1:RX_DE==0 e.g., DTR trancever is on.

on the second I2C channel that is for SMBus's i2c block commands

``` 
picocom -b 38400 /dev/ttyUSB0
/0/iaddr 42
{"address":"0x2A"}
/0/ibuff 51,38
{"txBuffer[2]":[{"data":"0x33"},{"data":"0x26"}]}
/0/iwrite
{"returnCode":"success"}
/0/ibuff 51
{"txBuffer[1]":[{"data":"0x33"}]}
/0/iread? 2
{"rxBuffer":[{"data":"0x33"},{"data":"0x26"}]}
```


## Cmd 51 from a Raspberry Pi read trancever control bits

Set trancever control bits (HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE) durring test_mode with an R-Pi over SMBus.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# end the test_mode data byte is replaced with the recoverd trancever control
bus.write_i2c_block_data(42, 51, [38])
print(bus.read_i2c_block_data(42, 51, 2))
[51, 38]
``` 


