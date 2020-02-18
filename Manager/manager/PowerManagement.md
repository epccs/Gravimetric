# Power Management Commands

32..47 (Ox20..0x2F | 0b00100000..0b00111111)

32. adc[channel] (uint16_t: send channel (ALT_I, ALT_V,PWR_I,PWR_V), return adc reading)
33. calMap[channelMap.cal_map[channel]] (uint8_t+uint32_t: send channel (ALT_I+CALIBRATION_SET) and float (as uint32_t), return channel and calibration)
34. not used
35. not used
36. analogTimedAccumulation for (uint32_t: send channel (ALT_IT,PWR_IT), return reading)
37. not used
38. Analog referance for EXTERNAL_AVCC (float: passed as four bytes or uint32_t)
39. Analog referance for INTERNAL_1V1 (float: passed as four bytes or uint32_t)

## Cmd 32 from the application controller /w i2c-debug running read analog channels

Needs three bytes from I2C. Example shows command followed by the ADC high byte and the low byte of a buffered reading from channel 7 (PWR_V).

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 32,0,7
{"txBuffer[3]":[{"data":"0x20"},{"data":"0x0"},{"data":"0x7"}]}
/1/iread? 3
{"rxBuffer":[{"data":"0x20"},{"data":"0x1"},{"data":"0x66"}]}
/1/ibuff 32,0,6
{"txBuffer[3]":[{"data":"0x20"},{"data":"0x00"},{"data":"0x06"}]}
/1/iread? 3
{"rxBuffer":[{"data":"0x20"},{"data":"0x0"},{"data":"0x14"}]}
``` 

ALT_I is measured with Analog channel 0 from a 0.018 Ohm sense resistor that has a pre-amp with gain of 50 connected.

ALT_V is measured with Analog channel 1 from a divider with 100k and 10k.

PWR_V is measured with Analog channel 7 from a divider with 100k and 15.8k, its two bytes are from analogRead and sum to 358 (e.g., (2**8)*0x1 + 0x66).  The corrected value is about 12.8V (e.g., (analogRead/1024)*referance*((100+15.8)/15.8) ) where the referance is 5V.

PWR_I is measured with Analog channel 6 from a 0.068 Ohm sense resistor that has a pre-amp with gain of 50 connected, its two bytes are from analogRead and sum to 20 (e.g., 0x14). The corrected value is about 0.029A (e.g., (analogRead/1024)*referance/(0.068*50.0) ) where the referance is 5V.


## Cmd 32 from a Raspberry Pi read analog channels

An R-Pi host can use its Linux SMBus driver to read read analog from PWR_V.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# what is the analog from PWR_V high and low byte.
bus.write_i2c_block_data(42, 32, [0,7])
bus.read_i2c_block_data(42, 32, 3)
[32, 1, 102]
``` 


## Cmd 33 from the application controller /w i2c-debug running read analog channels

Needs six bytes from I2C. Example shows command followed by channel followed by calibration value (float).

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 33,7
{"txBuffer[3]":[{"data":"0x21"},{"data":"0x7"},{"data":"0x7"}]}
/1/ibuff 0,0,0,0
{"txBuffer[6]":[{"data":"0x21"},{"data":"0x7"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 6
this is broken atm
{"rxBuffer":[{"data":"0x21"},{"data":"0x7"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
#{"rxBuffer":[{"data":"0x21"},{"data":"0x7"},{"data":"0x40"},{"data":"0xEA"},{"data":"0x88"},{"data":"0x1A"}]}
``` 

note: i2c-debug can add up to five arguments at a time into txBuffer.

ALT_I is measured with Analog channel 0 from a 0.018 Ohm sense resistor that has a pre-amp with gain of 50 connected.

ALT_V is measured with Analog channel 1 from a divider with 100k and 10k.

PWR_V is measured with Analog channel 7 from a divider with 100k and 15.8k, its calibration value is passed in four bytes starting with the high byte. Use it with the referance to correct the analogRead value (e.g., (analogRead/1024)*referance*calibrationRead.

``` python
from struct import *
# how does python pack a float?
unpack('BBBB', pack('f', (100+15.8)/15.8) )
(26, 136, 234, 64)
# shows that python packing order is high byte last, but my I2C data is high byte first (flip order). 
unpack('f', pack('BBBB', 0x1A, 0x88, 0xEA, 0x40))
(7.329113960266113,)
```

PWR_I is measured with Analog channel 6 from a 0.068 Ohm sense resistor that has a pre-amp with gain of 50 connected, its two bytes are from analogRead and sum to 20 (e.g., 0x14). The corrected value is about 0.029A (e.g., (analogRead/1024)*referance/(0.068*50.0) ) where the referance is 5V.

Add CALIBRATION_SET (0x80) to channel to save the calibration value sent (otherwise it is ignored).


## Cmd 36 from the application controller /w i2c-debug running read analog timed accumulation.

Needs five bytes from I2C. They are command followed by a UINT32 from a buffered ADC's timed accumulation.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 36,0,0,0,6
{"txBuffer[5]":[{"data":"0x24"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x6"}]}
/1/iread? 5
{"rxBuffer":[{"data":"0x24"},{"data":"0x9"},{"data":"0x5"},{"data":"0x39"},{"data":"0x76"}]}
``` 

The PWR_IT four bytes sum to 15,1337,334 (e.g., 9*(2**24) + 5*(2**16) + 57*(2**8) + 118). PWR_I is measured with analog channel 6 on a 0.068 Ohm sense resistor that has a pre-amp with gain of 50 connected and a referance of 5V. PWR_IT is accumulated every 10mSec, so use PWR_I correction and divide by 1000*3600/100 to get 6.037mAHr (e.g., (accumulated/1024)*referance/(0.068*50.0)/36000)). Clearly it is running to fast, but it seems to work.


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

