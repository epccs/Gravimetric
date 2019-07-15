# Point To Point Commands

16..31 (Ox10..0x1F | 0b00010000..0b00011111)

16. set arduino_mode 
17. read arduino_mode

## Cmd 16 from a controller /w i2c-debug set p2p mode

set arduino_mode so LOCKOUT_DELAY and BOOTLOADER_ACTIVE last forever when the host RTS toggles active. Arduino IDE can then make p2p connections to the bootload address.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 16,1
{"txBuffer[2]":[{"data":"0x10"},{"data":"0x01"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x10"},{"data":"0x01"}]}
``` 

That was silly since i2c-debug will read the RPUbus address over I2C and cause the manager to send a normal mode (e.g., point to multi-point) on the DTR pair. 


## Cmd 16 from a Raspberry Pi set p2p mode

An R-Pi host can do it with SMBus, lets try with two RPUpi boards.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# what is the bootload address
bus.write_i2c_block_data(42, 2, [255])
bus.read_i2c_block_data(42, 2, 2)
[2, 48]
# what is my address
bus.write_i2c_block_data(42, 0, [255])
bus.read_i2c_block_data(42, 0, 2)
[0, 50]
# set the bootload address to my address
bus.write_i2c_block_data(42, 3, [ord('2')])
bus.read_i2c_block_data(42, 3, 2)
[3, 50]
# clear the host lockout status bit so serial from this host can work
bus.write_i2c_block_data(42, 7, [0])
print(bus.read_i2c_block_data(42, 7, 2))
[7, 0]
exit()
# on an RPUno load the blinkLED application which does not read the bus address
git clone https://github.com/epccs/RPUno
cd /RPUno/BlinkLED
make
make bootload
# now back to 
python3
import smbus
bus = smbus.SMBus(1)
# and set the arduino_mode
bus.write_i2c_block_data(42, 16, [1])
print(bus.read_i2c_block_data(42, 16, 2))
[16, 1]
# the R-Pi host can now connect by serial so LOCKOUT_DELAY and BOOTLOADER_ACTIVE last forever
``` 

Now the R-Pi can connect to serial in P2P mode.

``` 
picocom -b 38400 /dev/ttyAMA0
``` 

At this time, the point to point mode persists, I will sort out more details when they are needed.


## Cmd 17 from a Raspberry Pi read p2p mode

An R-Pi host can use SMBus to check for p2p mode.

```
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# is it in p2p mode (a zero shows that the running app has read the rpubus address)
bus.write_i2c_block_data(42, 17, [255])
print(bus.read_i2c_block_data(42, 17, 2))
[65, 0]
```
