# Point To Multi-Point Commands

0..15 (Ox00..0xF | 0b00000000..0b00001111)

0. read the RPU_BUS address and activate normal mode (broadcast if localhost_active).
1. set the RPU_BUS address and write it to EEPROM.
2. read the address sent when DTR/RTS toggles.
3. write the address that will be sent when DTR/RTS toggles
4. read shutdown switch (the ICP1 pin has a weak pull-up and a momentary switch).
5. set shutdown switch (pull down ICP1 for SHUTDOWN_TIME to cause the host to halt).
6. reads status bits [0:DTR readback timeout, 1:twi transmit fail, 2:DTR readback not match, 3:host lockout].
7. writes (or clears) status.


## Cmd 0 from a controller /w i2c-debug read the shield address

The local RPU address can be read.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 0,255
{"txBuffer[2]":[{"data":"0x0"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x0"},{"data":"0x31"}]}
``` 


## Cmd 0 from a Raspberry Pi read the shield address

The host can read the local RPU_ADDRESS with the I2C1 port.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
bus.write_i2c_block_data(42, 0, [255])
print(bus.read_i2c_block_data(42, 0, 2))
[0, 49]
``` 


## Cmd 1 from a controller /w i2c-debug set RPU_BUS address

__Warning:__ this changes eeprom, flashing with ICSP does not clear eeprom due to fuse setting.

Using an RPUno and an RPUftdi shield, connect another RPUno with i2c-debug firmware to the RPUpi shield that needs its address set. The default RPU_BUS address can be changed from '1' to any other value. 

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 1,50
{"txBuffer[2]":[{"data":"0x1"},{"data":"0x32"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x1"},{"data":"0x32"}]}
``` 

The RPU program normally reads the address during setup in which case it needs a reset to get the update. The reset can be done by setting its bootload address (bellow).

```
/2/id?
{"id":{"name":"I2Cdebug^1","desc":"RPUno (14140^9) Board /w atmega328p","avr-gcc":"5.4.0"}}
``` 


## Cmd 2 from a controller /w i2c-debug read the address sent when DTR/RTS toggles

I2C0 can be used by the RPU to Read the local bootload address that it will send when a host connects to it.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 2,255
{"txBuffer[2]":[{"data":"0x2"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x2"},{"data":"0x30"}]}
``` 

Address 0x30 is ascii '0' so the RPU at that location will be reset and connected to the serial bus when a host connects to the shield at address '1'. 


## Cmd 2 from a Raspberry Pi read the address sent when DTR/RTS toggles

I2C1 can be used by the host to Read the local bootload address that it will send when a host connects to it.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
bus.write_i2c_block_data(42, 2, [255])
print(bus.read_i2c_block_data(42, 2, 2))
[2, 48]
``` 

Address 48 is 0x30 or ascii '0' so the RPU at that location will be reset and connected to the serial bus when the Raspberry Pi connects to the shield at this location (use command 1 to find the local address).


## Cmd 3 from a controller /w i2c-debug set the bootload address

__Note__: this valuse is not saved in eeprom so a power loss will set it back to '0'.

I2C0 can be used by the RPU to set the local bootload address that it will send when a host connects to it. When DTR/RTS toggles send ('2' is 0x32 is 50).

```
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 3,50
{"txBuffer[2]":[{"data":"0x3"},{"data":"0x32"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x3"},{"data":"0x32"}]}
``` 

exit picocom with C-a, C-x. 

Connect with picocom again. 

``` 
picocom -b 38400 /dev/ttyUSB0
``` 

This will toggle RTS on the RPUpi shield and the manager will send 0x32 on the DTR pair. The RPUpi shield should blink slow to indicate a lockout, while the shield with address '2' blinks fast to indicate bootloader mode. The lockout timeout LOCKOUT_DELAY can be adjusted in firmware.


## Cmd 3 from a Raspberry Pi set the bootload address

__Note__: this valuse is not saved in eeprom so a power loss will set it back to '0'.

I2C1 can be used by the host to set the local bootload address that it will send when a host connects to it. When DTR/RTS toggles send ('2' is 0x32 is 50).

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
bus.write_i2c_block_data(42, 3, [50])
print(bus.read_i2c_block_data(42, 3, 2))
[3, 50]
``` 


## Cmd 4 from a controller /w i2c-debug read if HOST shutdown detected

To check if host got a manual hault command (e.g. if the shutdown button got pressed) send the I2C shutdown command 4 (first byte), with place holder data (second byte). This clears shutdown_detected flag that was used to keep the LED_BUILTIN from blinking.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 4,255
{"txBuffer[2]":[{"data":"0x4"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x4"},{"data":"0x1"}]}
/1/ibuff 4,255
{"txBuffer[2]":[{"data":"0x4"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x4"},{"data":"0x0"}]}
``` 

The above used a remote host and shield.

Second value in rxBuffer has shutdown_detected value 0x1, it is cleared after reading.


## Cmd 5 from a controller /w i2c-debug set HOST to shutdown

To hault the host send the I2C shutdown command 5 (first byte), with data 1 (second byte) which sets shutdown_started, clears shutdown_detected and pulls down the SHUTDOWN (ICP1) pin. The shutdown_started flag is also used to stop blinking of the LED_BUILTIN to reduce power usage noise so that the host power usage can be clearly seen.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 5,1
{"txBuffer[2]":[{"data":"0x5"},{"data":"0x1"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x5"},{"data":"0x1"}]}
``` 

The above used a remote host and shield.


## Cmd 6 from a controller /w i2c-debug read status bits

I2C0 can be used by the RPU to read the local status bits.

0. DTR readback timeout
1. twi transmit fail 
2. DTR readback not match
3. host lockout

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 6,255
{"txBuffer[2]":[{"data":"0x6"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x6"},{"data":"0x8"}]}
``` 


## Cmd 6 from a Raspberry Pi read status bits

I2C1 can be used by the host to read the local status bits.

0. DTR readback timeout
1. twi transmit fail 
2. DTR readback not match
3. host lockout

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
bus.write_i2c_block_data(42, 6, [255])
print(bus.read_i2c_block_data(42, 6, 2))
[6, 8]
``` 

Bit 3 is set so the host can not connect until that has been cleared.


## Cmd 7 from a controller /w i2c-debug set status bits

I2C0 can be used by the RPU to set the local status bits.

0. DTR readback timeout
1. twi transmit fail 
2. DTR readback not match
3. host lockout

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 7,8
{"txBuffer[2]":[{"data":"0x7"},{"data":"0x0"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x7"},{"data":"0x0"}]}
``` 

## Cmd 7 from a Raspberry Pi sets status bits

I2C1 can be used by the host to set the local status bits.

0. DTR readback timeout
1. twi transmit fail 
2. DTR readback not match
3. host lockout

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
bus.write_i2c_block_data(42, 7, [0])
print(bus.read_i2c_block_data(42, 7, 2))
[7, 0]
exit()
picocom -b 38400 /dev/ttyAMA0
...
Terminal ready
/1/id?
# C-a, C-x.
``` 

The Raspberry Pi can bootload a target on the RPU serial bus.

