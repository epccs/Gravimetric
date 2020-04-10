# Point To Multi-Point Commands

0..15 (Ox00..0xF | 0b00000000..0b00001111)

0. access the multi-drop address, range 48..122 (ASCII '0'..'z').
1. not used.
2. access the multi-drop bootload address that will be sent when DTR/RTS toggles.
3. not used.
4. access shutdown_detect, manager MCU_IO_SHUTDOWN has a weak pull-up and a momentary switch.
5. not used.
6. read status bits.
7. write (or clear) status.

status bits: 

0. DTR (management pair) readback timeout
1. twi slave transmit fail
2. DTR readback not match
3. host lockout
4. alternate power enable (ALT_EN)
5. SBC power enable (PIPWR_EN), use to restart SBC after a shutdown with command 5
6. Day-Night state fail (e.g. > 20hr of day or night), clear will restart it.
7. Update bit, if set change the other bits.


## Cmd 0 from the application controller /w i2c-debug access the serial multi-drop address

The application controller can read the local serial multi-drop address from the manager.

This will activate (broadcast) normal multi-drop mode (e.g., after bootload mode or point to point mode).

``` 
# picocom -b 38400 /dev/ttyUSB0
picocom -b 38400 /dev/ttyAMA0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 0,255
{"txBuffer[2]":[{"data":"0x0"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x0"},{"data":"0x31"}]}
``` 

The out of range data byte above is ignored. If the sent data byte is in the range 48..122 (ASCII '0'..'z'), then it will be saved in EEPROM, and normal mode will not be broadcast.

__Warning:__ this changes EEPROM on the manager, flashing the manager ICSP may clear its eeprom.

``` 
/1/ibuff 0,50
{"txBuffer[2]":[{"data":"0x0"},{"data":"0x32"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x0"},{"data":"0x31"}]}
/1/ibuff 0,255
{"txBuffer[2]":[{"data":"0x0"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x0"},{"data":"0x32"}]}
``` 

The old value was returned, but the second read shows the new value (and caused a normal mode broadcast)

Although the manager immediately knows its new address, the application controller program needs to read the new address; I do this once during the applications setup section. I can reset the application controller to load the update. The reset can be done by setting the bootload address (bellow) and then opening the port with picocom.

```
/2/id?
{"id":{"name":"I2C0debug^1","desc":"Gravimetric (17341^1) Board /w ATmega324pb","avr-gcc":"5.4.0"}}
``` 


## Cmd 0 from a Raspberry Pi access the serial multi-drop address

The local host can read the local serial multi-drop address from the manager.

This does not active normal mode.

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


## Cmd 1 is not used

Will be repurposed.


## Cmd 2 from the application controller /w i2c-debug access the bootload address (48..122) sent when serial handshake RTS toggless.

The application controller can access the serial multi-drop bootload address that is sent when the local host opens its serial port (RTS goes active).

__Note__: this valuse is not saved in eeprom so a power loss will set it back to default ('0').

``` 
picocom -b 38400 /dev/ttyAMA0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 2,255
{"txBuffer[2]":[{"data":"0x2"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x2"},{"data":"0x30"}]}
/1/ibuff 2,49
{"txBuffer[2]":[{"data":"0x2"},{"data":"0x31"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x2"},{"data":"0x31"}]}
``` 

The out of range address above was ignored. If the sent bootload address is in the range 48..122 (ASCII '0'..'z') then it will be used.

Address 0x30 is ASCII '0'. The manager with that address will reset its application controller, and the serial multi-drop bus will switch to a point to point connection between the host and the application controller connected to that manager. The point to point serial allows the uploader tool to send a new application image to the bootloader, which places it in the flash execution memory. 

exit picocom with C-a, C-x. 

Connect with picocom again. 

``` 
picocom -b 38400 /dev/ttyAMA0
``` 

Opening the serial port will toggle the RTS from the host, and the manager will see it and send 0x31 on the DTR pair. The manager(s) that do not have that address should blink there LED slow to indicate lockout, while the manager with address '1' blinks fast to indicate bootloader mode. 


## Cmd 2 from a Raspberry Pi access the bootload address (48..122) sent when serial handshake RTS toggles.

The local host can access the serial multi-drop bootload address that is sent when the local host opens its serial port.

__Note__: this valuse is not saved in eeprom so a power loss will set it back to default ('0').

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

The out of range address above was ignored. If the sent bootload address is in the range 48..122 (ASCII '0'..'z') then it will be used.

Address 48 is 0x30 or ASCII '0'. The manager with that address will reset its application controller, and the serial multi-drop bus will switch to a point to point connection between the host and the application controller connected to that manager. The point to point serial allows the uploader tool to send a new application image to the bootloader, which places it in the flash execution memory. 


## Cmd 4 from the application controller /w i2c-debug access host shutdown detected.

The application controller can check if the host got a manual halt command (e.g., if the shutdown button got pressed). Reading will clear the shutdown_detected flag that was used to keep the LED_BUILTIN from blinking.

``` 
picocom -b 38400 /dev/ttyAMA0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 4,0
{"txBuffer[2]":[{"data":"0x4"},{"data":"0x0"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x4"},{"data":"0x1"}]}
/1/ibuff 4,0
{"txBuffer[2]":[{"data":"0x4"},{"data":"0x0"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x4"},{"data":"0x0"}]}
``` 

The above interaction with a remote host was over the multi-drop serial. The second reading shows that shutdown_detected cleared.

``` 
/1/ibuff 4,1
{"txBuffer[2]":[{"data":"0x4"},{"data":"0x1"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x4"},{"data":"0x0"}]}
/1/ibuff 4,0
{"txBuffer[2]":[{"data":"0x4"},{"data":"0x0"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x4"},{"data":"0x1"}]}
``` 

The data byte == 1 will cause shutdown and set the detected flag after SHUTDOWN_TIME time has elapsed.


## Cmd 5 is not used.

Will be repurposed.


## Cmd 6 from a controller /w i2c-debug access status bits

The application controller can access the status of the manager. 

status bits: 

0. DTR (management pair) readback timeout
1. twi slave transmit fail
2. DTR readback not match
3. host lockout
4. alternate power enable (ALT_EN)
5. SBC power enable (PIPWR_EN), use to restart SBC after a shutdown
6. Day-Night state fail (e.g. > 20hr of day or night), clear will restart it.
7. Update bit, if set change the other bits.

``` 
picocom -b 38400 /dev/ttyAMA0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 6,255
{"txBuffer[2]":[{"data":"0x6"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x6"},{"data":"0x8"}]}
``` 


## Cmd 6 from a Raspberry Pi access status bits

The local host can read status bits.

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

Bit 3 is set so the host lockout is set until that has been cleared.

``` 
bus.write_i2c_block_data(42, 6, [0x80])
print(bus.read_i2c_block_data(42, 6, 2))
[6, 0]
exit()
picocom -b 38400 /dev/ttyAMA0
...
Terminal ready
/1/id?
# C-a, C-x.
``` 

The Raspberry Pi has cleared the host lockout bit (3) and can now bootload a target on the multi-drop serial bus. Once PIPWR_EN is set it will not clear (see command 5 for shutdown). 

