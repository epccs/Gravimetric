# Power Management Commands

32..47 (Ox20..0x2F | 0b00100000..0b00111111)

32. Analog channel 0 for ALT_I (uint16_t)
33. Analog channel 1 for ALT_V (uint16_t)
34. Analog channel 6 for PWR_I (uint16_t)
35. Analog channel 7 for PWR_V (uint16_t)
36. Analog timed accumulation for ALT_IT (uint32_t)
37. Analog timed accumulation for PWR_IT (uint32_t)
38. Analog referance for EXTERNAL_AVCC (uint32_t)
39. Analog referance for INTERNAL_1V1 (uint32_t)

## Cmd 32..35 from the application controller /w i2c-debug running read analog channels

Read two bytes from I2C. They are the high byte and low byte of a buffered ADC reading from channel 7 (PWR_V).

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 35,255,255
{"txBuffer[3]":[{"data":"0x23"},{"data":"0xFF"},{"data":"0xFF"}]}
/1/iread? 3
{"rxBuffer":[{"data":"0x23"},{"data":"0x1"},{"data":"0x66"}]}
/1/ibuff 34,255,255
{"txBuffer[3]":[{"data":"0x22"},{"data":"0xFF"},{"data":"0xFF"}]}
/1/iread? 3
{"rxBuffer":[{"data":"0x22"},{"data":"0x0"},{"data":"0x14"}]}
``` 

ALT_I is from a 0.018 Ohm sense resistor that has a pre-amp with gain of 50 connected.

ALT_V is from a divider with 100k and 10k.

PWR_V is from a divider with 100k and 15.8k, its two bytes are from analogRead and sum to 358 (e.g., (2**8)*0x1 + 0x66).  The corrected value is about 12.8V (e.g., (analogRead/1024)*referance*((100+15.8)/15.8) ) where the referance is 5V.

PWR_I is from a 0.068 Ohm sense resistor that has a pre-amp with gain of 50 connected, its two bytes are from analogRead and sum to 20 (e.g., 0x14). The corrected value is about 0.029A (e.g., (analogRead/1024)*referance/(0.068*50.0) ) where the referance is 5V.


## Cmd 32..35 from a Raspberry Pi read analog channels

An R-Pi host can use its Linux SMBus driver to read read analog from ALT_I.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# what is the analog from ALT_I high and low byte.
bus.write_i2c_block_data(42, 35, [255,255])
bus.read_i2c_block_data(42, 35, 3)
[32, 1, 102]
``` 

## Cmd 36 and 37 from the application controller /w i2c-debug running read analog timed accumulation.

Read four bytes from I2C. They are bytes to a UINT32 from a buffered ADC's timed accumulation reading.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 37,255,255,255,255
{"txBuffer[5]":[{"data":"0x25"},{"data":"0xFF"},{"data":"0xFF"},{"data":"0xFF"},{"data":"0xFF"}]}
/1/iread? 5
{"rxBuffer":[{"data":"0x25"},{"data":"0x9"},{"data":"0x5"},{"data":"0x39"},{"data":"0x76"}]}
``` 

The PWR_IT four bytes sum to 15,1337,334 (e.g., 9*(2**24) + 5*(2**16) + 57*(2**8) + 118). PWR_I is from a 0.068 Ohm sense resistor that has a pre-amp with gain of 50 connected and a referance of 5V. PWR_IT is accumulated every 10mSec, so use PWR_I correction and divide by 1000*3600/100 to get 6.037mAHr (e.g., (accumulated/1024)*referance/(0.068*50.0)/36000)). Clearly it is running to fast, but it seems to work.


## Cmd 38 and 39 from the application controller /w i2c-debug running read analog referance.

Read four bytes from I2C. They are from a buffered value mirrored in the manager's EEPROM. The value is ignored if out of range. Use 38 for EXTERNAL_AVCC, and 39 for INTERNAL_1V1 (not loaded by SelfTest).

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 38,255,255,255,255
{"txBuffer[5]":[{"data":"0x26"},{"data":"0xFF"},{"data":"0xFF"},{"data":"0xFF"},{"data":"0xFF"}]}
/1/iread? 5
{"rxBuffer":[{"data":"0x26"},{"data":"0x40"},{"data":"0xA0"},{"data":"0x0"},{"data":"0x0"}]}
``` 

The EXTERNAL_AVCC four bytes are a float so lets use python to see if they are right. 

``` python
from struct import *
# packing order is high byte last
unpack('f', pack('BBBB', 0x0, 0x0, 0xA0, 0x40))
```

Output is: (5.0,)

