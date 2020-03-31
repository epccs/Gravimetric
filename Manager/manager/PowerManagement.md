# Power Management Commands

32..47 (Ox20..0x2F | 0b00100000..0b00111111)

32. adc[channel] (uint16_t: send enum (ALT_I, ALT_V,PWR_I,PWR_V), return adc reading)
33. calMap[enum] (uint8_t+uint32_t: send enum (ALT_I+CALIBRATION_SET) and float (as uint32_t), return channel and calibration)
34. not used
35. not used
36. analogTimedAccumulation for (uint32_t: send channel (ALT_IT,PWR_IT), return reading)
37. not used
38. refMap[select] (uint8_t+uint32_t: send select (0x80:REF_SELECT_WRITEBIT+0:EXTERNAL_AVCC,1:INTERNAL_1V1) and float (as uint32_t), return select and referance)
39. not used

## Cmd 32 from the application controller /w i2c-debug running read analog channels

Needs three bytes from I2C. Example shows command followed by the ADC high byte and the low byte of a buffered reading from channel 7 (PWR_V).

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"master_address":"0x29"}
/1/ibuff 32,0,0
{"txBuffer[3]":[{"data":"0x20"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 3
*** wrong ***{"txBuffer":"wrt_success","rxBuffer":"rd_success","rxBuffer":[{"data":"0x20"},{"data":"0x1"},{"data":"0x66"}]}
/1/ibuff 32,0,1
{"txBuffer[3]":[{"data":"0x20"},{"data":"0x0"},{"data":"0x1"}]}
/1/iread? 3
{"txBuffer":"wrt_success","rxBuffer":"rd_success","rxBuffer":[{"data":"0x20"},{"data":"0x0"},{"data":"0x0"}]}
/1/ibuff 32,0,2
{"txBuffer[3]":[{"data":"0x20"},{"data":"0x00"},{"data":"0x02"}]}
/1/iread? 3
{"txBuffer":"wrt_success","rxBuffer":"rd_success","rxBuffer":[{"data":"0x20"},{"data":"0x0"},{"data":"0x7"}]}
/1/ibuff 32,0,3
{"txBuffer[3]":[{"data":"0x20"},{"data":"0x0"},{"data":"0x3"}]}
/1/iread? 3
{"txBuffer":"wrt_success","rxBuffer":"rd_success","rxBuffer":[{"data":"0x20"},{"data":"0x1"},{"data":"0x66"}]}
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
{"txBuffer[3]":[{"data":"0x21"},{"data":"0x7"}]}
/1/ibuff 0,0,0,0
{"txBuffer[6]":[{"data":"0x21"},{"data":"0x7"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 6
{"rxBuffer":[{"data":"0x21"},{"data":"0x7"},{"data":"0x3B"},{"data":"0xEA"},{"data":"0x88"},{"data":"0x1A"}]}
/1/ibuff 33,6
{"txBuffer[3]":[{"data":"0x21"},{"data":"0x6"}]}
/1/ibuff 0,0,0,0
{"txBuffer[6]":[{"data":"0x21"},{"data":"0x6"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 6
{"rxBuffer":[{"data":"0x21"},{"data":"0x6"},{"data":"0x39"},{"data":"0x96"},{"data":"0x96"},{"data":"0x96"}]}
``` 

note: i2c-debug can add up to five arguments at a time into txBuffer.

ALT_I is measured with Analog channel 0 from a 0.018 Ohm sense resistor that has a pre-amp with gain of 50 connected.

``` python
# default calibration value for each bit on ALT_I, which has 0.018 Ohm sense resistor and gain of 50
(1.0/(1<<10))/(0.068*50.0)
0.0002872242647058823
```

ALT_V is measured with Analog channel 1 from a divider with 100k and 10k.

``` python
# default calibration value for each bit on ALT_V, which is a divider of 100k and 10.0k
(1.0/(1<<10))*((100+10.0)/10.0)
0.0107421875
```

PWR_V is measured with Analog channel 7 from a divider with 100k and 15.8k, its calibration value is passed in four bytes starting with the high byte. Use it with the referance to correct the analogRead value (e.g., (analogRead/1024)*referance*calibrationRead.

``` python
# default calibration value for each bit on PWR_V, which is a divider of 100k and 15.8k
(1.0/(1<<10))*((100+15.8)/15.8)
0.0071573378
from struct import *
# how does python pack a float?
unpack('BBBB', pack('f', (1.0/(1<<10))*((100+15.8)/15.8)) )
(26, 136, 234, 59)
# shows that python packing order is high byte last, but my I2C data is high byte first (flip order). 
unpack('f', pack('BBBB', 0x1A, 0x88, 0xEA, 0x3B))
(0.0071573378,)
# precision is 6..7 decimal digits
```

PWR_I is measured with Analog channel 6 from a 0.068 Ohm sense resistor that has a pre-amp with gain of 50 connected, its two bytes are from analogRead and sum to 20 (e.g., 0x14). The corrected value is about 0.029A (e.g., (analogRead/1024)*referance/(0.068*50.0) ) where the referance is 5V.

``` python
# default calibration value for each bit on PWR_I, which has 0.068 Ohm sense resistor and gain of 50
(1.0/(1<<10))/(0.068*50.0)
0.0002872242647058823
unpack('f', pack('BBBB', 0x96, 0x96, 0x96, 0x39))
(0.0002872242475859821,)
```

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


## Cmd 38 from the application controller /w i2c-debug running read analog referance.

Needs six bytes from I2C. Example shows command followed by select followed by referance value (float). Use select=0 for EXTERNAL_AVCC, and select=1 for INTERNAL_1V1 (SelfTest needs to be update to save EXTERNAL_AVCC,but INTERNAL_1V1 is not used at this time).

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 38,0
{"txBuffer[3]":[{"data":"0x26"},{"data":"0x0"}]}
/1/ibuff 0,0,0,0
{"txBuffer[6]":[{"data":"0x26"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 6
{"rxBuffer":[{"data":"0x26"},{"data":"0x0"},{"data":"0x40"},{"data":"0xA0"},{"data":"0x0"},{"data":"0x0"}]}
/1/ibuff 38,1
{"txBuffer[3]":[{"data":"0x26"},{"data":"0x1"}]}
/1/ibuff 0,0,0,0
{"txBuffer[6]":[{"data":"0x26"},{"data":"0x1"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 6
{"rxBuffer":[{"data":"0x26"},{"data":"0x1"},{"data":"0x3F"},{"data":"0x8A"},{"data":"0x3D"},{"data":"0x71"}]}
```

For select = 0 the value of EXTERNAL_AVCC (four bytes) are returned as a float so lets use python to see if they are right. 

``` python
from struct import *
# packing order is high byte last
unpack('f', pack('BBBB', 0x0, 0x0, 0xA0, 0x40))
(5.0,)
unpack('f', pack('BBBB', 0x71, 0x3D, 0x8A, 0x3F))
(1.0800000429153442,)
```

