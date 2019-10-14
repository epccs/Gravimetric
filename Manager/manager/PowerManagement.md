# Power Management Commands

32..47 (Ox20..0x2F | 0b00100000..0b00111111)

32. Analog channel 0 for ALT_I (uint16_t)
33. Analog channel 1 for ALT_V (uint16_t)
34. Analog channel 6 for PWR_I (uint16_t)
35. Analog channel 7 for PWR_V (uint16_t)
36. Analog timed accumulation for ALT_IT (uint32_t)
37. Analog timed accumulation for PWR_IT (uint32_t)
38. Analog referance for EXTERNAL_AVCC (uint32_t)
39. Day-Night state machine based on ALT_V

## Cmd 32 from the application controller /w i2c-debug running

Read two bytes from I2C. They are the high byte and low byte of a buffered ADC reading from channel 0 (ALT_I).

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 32,255,255
{"txBuffer[3]":[{"data":"0x20"},{"data":"0xFF"},{"data":"0xFF"}]}
/1/iread? 3
{"rxBuffer":[{"data":"0x20"},{"data":"0x00"},{"data":"0x3F"}]}
``` 

## Cmd 32 from a Raspberry Pi read analog from ALT_I

An R-Pi host can use its Linux SMBus driver to read read analog from ALT_I.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# what is the analog from ALT_I high and low byte.
bus.write_i2c_block_data(42, 32, [255,255])
bus.read_i2c_block_data(42, 32, 3)
[32, 0, 63]
``` 

## Cmd 37 from the application controller /w i2c-debug running

Every ten mSec the ADC value is added to the timed accumulation value. To get the corrected value it should be scaled (same as PWR_I) with the high side current sense gain (50) and the sense value is from 0.068 ohm. After an hour 360,000 samples are accumulated. If each was for 735mA then the adc reads 512 and the accumulation will be 184320000.  

``` python
# referance used to scale ADC
ref_extern_avcc = 5.0
# accumulate an hour of readings at 0.735A
accumulate_pwr_ti = 512 * 100 * 3600
# correct timed accumulation to mAHr
accumulate_pwr_ti_mAHr = accumulate_pwr_ti*((ref_extern_avcc)/1024.0)/(0.068*50.0)/360
```

