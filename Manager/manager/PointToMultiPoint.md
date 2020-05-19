# Point To Multi-Point Commands

0..15 (Ox00..0xF | 0b00000000..0b00001111)

0. access the multi-drop address, range 48..122 (ASCII '0'..'z').
1. access status bits.
2. access the multi-drop bootload address that will be sent when DTR/RTS toggles.
3. access arduino_mode.
4. not used. set Host Shutdown i2c callback (set shutdown_callback_address, report hostshutdown_state cmd).
5. not used. Access shutdown_halt_curr_limit (uint16). I2C data: cmd,rd-wr,high-byte,low-byte.
6. not used. Access shutdown_halt_ttl_limit, shutdown_delay_limit, shutdown_wearleveling_limit(uint32). I2C data: cmd,rd-wr+offset[0..2],bits[31..24],bits[23..16],bits[15..8],bits[7..0].
7. not used.


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


## Cmd 1 from a controller /w i2c-debug access status bits

The application controller can access the status of the manager. 

status bits: 

0. DTR (management pair) readback timeout
1. twi slave transmit fail
2. DTR readback not match
3. host lockout
4. not used.
5. not used.
6. not used.
7. Update bit, if set change the other bits.

``` 
#picocom -b 38400 /dev/ttyUSB0
picocom -b 38400 /dev/ttyAMA0
/1/iaddr 41
{"master_address":"0x29"}
/1/ibuff 1,0
{"txBuffer[2]":[{"data":"0x1"},{"data":"0x0"}]}
/1/iread? 2
{"txBuffer":"wrt_success","rxBuffer":"rd_success","rxBuffer":[{"data":"0x1"},{"data":"0x8"}]}
``` 


## Cmd 1 from a Raspberry Pi access status bits

The local host can read status bits.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
bus.write_i2c_block_data(42, 1, [0])
print(bus.read_i2c_block_data(42, 1, 2))
[1, 8]
``` 

Bit 3 is set so the host lockout is set until that has been cleared.

``` 
bus.write_i2c_block_data(42, 1, [0x80])
print(bus.read_i2c_block_data(42, 1, 2))
[6, 0]
exit()
picocom -b 38400 /dev/ttyAMA0
...
Terminal ready
/1/id?
# C-a, C-x.
``` 

The Raspberry Pi has cleared the host lockout bit (3) and can now bootload a target (or do p2p) on the multi-drop serial bus.


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


## Cmd 3 from a controller /w i2c-debug set p2p mode

set arduino_mode so LOCKOUT_DELAY and BOOTLOADER_ACTIVE last forever when the host RTS toggles active. Arduino IDE can then make p2p connections to the bootload address.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 3,1
{"txBuffer[2]":[{"data":"0x3"},{"data":"0x01"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x3"},{"data":"0x01"}]}
``` 

The i2c-debug applicaiton will read the address on i2c which will cause the manager to send a RPU_NORMAL_MODE signal on the DTR pair and end the p2p connection, to stay in p2p the bus address must not be read.


## Cmd 3 from a Raspberry Pi set p2p mode

An R-Pi host can change the the bootload address with SMBus. The bootload addres is also used for the point to point mode. 

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# what is the bootload address I will send
bus.write_i2c_block_data(42, 2, [0])
bus.read_i2c_block_data(42, 2, 2)
[2, 48]
# that is ascii '0', but wait what is my address
bus.write_i2c_block_data(42, 0, [0])
bus.read_i2c_block_data(42, 0, 2)
[0, 50]
# that is ascii '2', so to bootlaod my address
bus.write_i2c_block_data(42, 2, [ord('2')])
bus.read_i2c_block_data(42, 2, 2)
[2, 50]
# also clear the host lockout status bit so serial /dev/ttyAMA0 from this host can bootload
bus.write_i2c_block_data(42, 1, [0])
print(bus.read_i2c_block_data(42, 1, 2))
[6, 0]
exit()
# load the blinkLED application which does not read the bus address
git clone https://github.com/epccs/Gravimetric/
cd /Gravimetric/Applications/BlinkLED
make all
make bootload
# now back to 
python3
import smbus
bus = smbus.SMBus(1)
# and set the arduino_mode
bus.write_i2c_block_data(42, 3, [1])
print(bus.read_i2c_block_data(42, 3, 2))
[3, 0]
# arduino_mode will read back after it has been set over the DTR pair (sending zero does nothing)
bus.write_i2c_block_data(42, 3, [0])
print(bus.read_i2c_block_data(42, 3, 2))
[3, 1]
# the R-Pi host can now connect by serial so LOCKOUT_DELAY and BOOTLOADER_ACTIVE last forever
``` 

Now the R-Pi can connect to serial in P2P mode.

``` 
picocom -b 38400 /dev/ttyAMA0
``` 

At this time, the point to point mode persists, I will sort out more details when they are needed.


## Cmd 4 is not used.

Will be repurposed.

## Cmd 5 is not used.

Will be repurposed.


## Cmd 6 is not used.

repurpose

