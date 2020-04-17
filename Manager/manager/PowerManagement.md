# Power Management Commands

32..47 (Ox20..0x2F | 0b00100000..0b00111111)

32. adc[channel] (uint16_t: send enum (ALT_I, ALT_V,PWR_I,PWR_V), return adc reading)
33. access channel calibration value
34. not used
35. not used
36. analogTimedAccumulation for (uint32_t: send channel (ALT_IT,PWR_IT), return reading)
37. not used
38. access analog referance
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
{"txBuffer":"wrt_success","rxBuffer":"rd_success","rxBuffer":[{"data":"0x20"},{"data":"0x0"},{"data":"0x0"}]}
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


## Cmd 33 from the application controller /w i2c-debug running access channel calibration value

Needs six bytes from I2C. Example shows command followed by channel followed by calibration value (float).

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"master_address":"0x29"}
/1/ibuff 33,0
{"txBuffer[2]":[{"data":"0x21"},{"data":"0x0"}]}
/1/ibuff 0,0,0,0
{"txBuffer[6]":[{"data":"0x21"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 6
{"txBuffer":"wrt_success","rxBuffer":"rd_success","rxBuffer":[{"data":"0x21"},{"data":"0x0"},{"data":"0x3A"},{"data":"0x8E"},{"data":"0x38"},{"data":"0xE4"}]}
/1/ibuff 33,1
{"txBuffer[3]":[{"data":"0x21"},{"data":"0x1"}]}
/1/ibuff 0,0,0,0
{"txBuffer[6]":[{"data":"0x21"},{"data":"0x1"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 6
{"txBuffer":"wrt_success","rxBuffer":"rd_success","rxBuffer":[{"data":"0x21"},{"data":"0x1"},{"data":"0x3C"},{"data":"0x30"},{"data":"0x0"},{"data":"0x0"}]}
/1/ibuff 33,2
{"txBuffer[3]":[{"data":"0x21"},{"data":"0x2"}]}
/1/ibuff 0,0,0,0
{"txBuffer[6]":[{"data":"0x21"},{"data":"0x2"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 6
{"txBuffer":"wrt_success","rxBuffer":"rd_success","rxBuffer":[{"data":"0x21"},{"data":"0x2"},{"data":"0x39"},{"data":"0x96"},{"data":"0x96"},{"data":"0x96"}]}
/1/ibuff 33,3
{"txBuffer[3]":[{"data":"0x21"},{"data":"0x3"}]}
/1/ibuff 0,0,0,0
{"txBuffer[6]":[{"data":"0x21"},{"data":"0x3"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 6
{"txBuffer":"wrt_success","rxBuffer":"rd_success","rxBuffer":[{"data":"0x21"},{"data":"0x3"},{"data":"0x3B"},{"data":"0xEA"},{"data":"0x88"},{"data":"0x1A"}]}
``` 

note: i2c-debug can add up to five arguments at a time into txBuffer.

ALT_I is read with cmd 33 and select 0 from manager's channel 0 on a 0.018 Ohm sense resistor that has a pre-amp with gain of 50 connected.

``` python
# default calibration value for each bit on ALT_I, which has 0.018 Ohm sense resistor and gain of 50
(1.0/(1<<10))/(0.018*50.0)
0.0010850694
from struct import *
unpack('f', pack('BBBB', 0xE4, 0x38, 0x8E, 0x3A))
(0.0010850694,)
```

ALT_V is read with cmd 33 and select 1 from manager's channel 1 from a divider with 100k and 10k.

``` python
# default calibration value for each bit on ALT_V, which is a divider of 100k and 10.0k
(1.0/(1<<10))*((100+10.0)/10.0)
0.0107421875
unpack('f', pack('BBBB', 0x0, 0x0, 0x30, 0x3C))
(0.0107421875,)
```

PWR_I is read with cmd 33 and select 2 from manager's channel 6 from a 0.068 Ohm sense resistor that has a pre-amp with gain of 50 connected.

``` python
# default calibration value for each bit on PWR_I, which has 0.068 Ohm sense resistor and gain of 50
(1.0/(1<<10))/(0.068*50.0)
0.0002872242647058823
unpack('f', pack('BBBB', 0x96, 0x96, 0x96, 0x39))
(0.0002872242475859821,)
```

PWR_V is read with cmd 33 and select 3 from manager's channel 7 from a divider with 100k and 15.8k, its calibration value is passed in four bytes starting with the high byte. Use it with the referance to correct the analogRead value (e.g., (analogRead/1024)*referance*calibrationRead.

``` python
# default calibration value for each bit on PWR_V, which is a divider of 100k and 15.8k
(1.0/(1<<10))*((100+15.8)/15.8)
0.0071573378
unpack('f', pack('BBBB', 0x1A, 0x88, 0xEA, 0x3B))
(0.0071573378,)
```

If bit 7 in select (see CAL_CHANNEL_WRITEBIT in manager) is set the value sent will replace what is in eeprom.


## Cmd 36 from the application controller /w i2c-debug running read analog timed accumulation.

Enumeration 0 (ADC_ENUM_ALT_I) and 2 (ADC_ENUM_PWR_I) are use to select a timed accumulation. 

Send five bytes on I2C. They are are a command followed by a UINT32 for a timed accumulation to be returned.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"master_address":"0x29"}
/1/ibuff 36,0,0,0,0
{"txBuffer[5]":[{"data":"0x24"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 5
{"rxBuffer":[{"data":"0x24"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/ibuff 36,0,0,0,2
{"txBuffer[5]":[{"data":"0x24"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x2"}]}
/1/iread? 5
{"rxBuffer":[{"data":"0x24"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x97"}]}
``` 

The ALT_I accumulation is called ALT_IT... WIP not tested yet

The PWR_I accumulation is called PWR_IT, the four bytes sum to 151 (0x97). PWR_I is measured with analog enum input 2 on a 0.068 Ohm sense resistor that has a pre-amp with gain of 50 connected and a referance of 5V. PWR_I (((adc)/1024)*referance/(0.068*50.0)) readings are accumulated every 10mSec (1000mA*3600Hr/100) and when they reach 1,000,000 one is added to PWR_IT so the corrections are ((accumulated*1000000)/1024)*referance/(0.068*50.0)/36000 gives about 6.02mAHr.


## Cmd 38 from the application controller /w i2c-debug running access analog referance.

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

If bit 7 in select (see REF_SELECT_WRITEBIT in manager) is set the value sent will replace what is in eeprom. 
