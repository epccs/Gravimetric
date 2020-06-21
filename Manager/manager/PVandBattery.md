# PV, DayNight and Battery Management Commands

16..31 (Ox10..0x1F | 0b00010000..0b00011111)

16. Battery manager, enable with callback address (i2c), and and comand number to send state callback value to.
17. Access battery manager uint16 values. battery_[high_limit|low_limit|host_limit]
18. Access battery manager uint32 values. alt_pwm_accum_charge_time
19. Set daynight i2c callbacks (set callback address, report daynight_state cmd, day event cmd, night event cmd).
20. Access daynight manager uint16 values. daynight_[morning_threshold|evening_threshold]
21. Access daynight manager uint32 values. daynight_[morning_debounce|evening_debounce|...]
22. not used.
23. not used.

## Cmd 16 from a controller /w i2c-debug to enable battery manager

Enable the battery manager. The first byte is a callback address (e.g., the slave address to use). The second byte is used as a routing number for the application to receive the battery manager state machine events. The last byte is used to enable = 1..255 / disable = 0 the battery manager. 

``` 
# I am using the bootload interface 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 16,49,4,1
{"txBuffer[3]":[{"data":"0x10"},{"data":"0x31"},{"data":"0x4"},{"data":"0x1"}]}
/1/iread? 4
{"rxBuffer":[{"data":"0x10"},{"data":"0x31"},{"data":"0x4"},{"data":"0x1"}]}
```


## Cmd 17 from a controller /w i2c-debug to access battery manager uint16 values.

battery_high_limit to stop PWM and turn off charging when battery is above this value.

battery_low_limit to start PWM when battery is above this value

battery_host_limit to turn off host when battery is bellow this value

``` C
// I2C command to access battery_[high_limit|low_limit|host_limit]
// I2C: byte[0] = 17, 
//      byte[1] = bit 7 is read/write 
//                bits 6..0 is offset to battery_[high_limit|low_limit|host_limit],
//      byte[2] = bits 15..8,
//      byte[3] = bits 7..0
```

To read battery_low_limit value.

``` 
# using the bootload interface 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 17,1,0,0
{"txBuffer[4]":[{"data":"0x11"},{"data":"0x1"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 4
{"rxBuffer":[{"data":"0x11"},{"data":"0x1"},{"data":"0x1"},{"data":"0x76"}]}
```

The defaults are found in battery_limits.h. To convert the value to the corrected voltage ( (( (2**8)*0x1 + 0x76)/1024)*5.0*((100+15.8)/15.8) = 13.38V). To save a battery_low_limit set bit 7 (write) in byte[1].

``` 
/1/ibuff 17,129,1,119
{"txBuffer[4]":[{"data":"0x11"},{"data":"0x81"},{"data":"0x1"},{"data":"0x77"}]}
/1/iread? 4
{"rxBuffer":[{"data":"0x11"},{"data":"0x81"},{"data":"0x1"},{"data":"0x76"}]}
/1/ibuff 17,1,0,0
{"txBuffer[4]":[{"data":"0x11"},{"data":"0x1"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 3
{"rxBuffer":[{"data":"0x11"},{"data":"0x1"},{"data":"0x1"},{"data":"0x77"}]}
```

The data sent was swapped with the default; the second exchange shows the updated value did change.


## Cmd 18 from the application controller /w i2c-debug to access battery manager uint32 values.

The alt_pwm_accum_charge_time value is the on time accumulated when PWR_V is between battery_high_limit and battery_low_limit.

``` C
// I2C command to access alt_pwm_accum_charge_time
// I2C: byte[0] = 18, 
//      byte[1] = bit 7 is read/write 
//                bits 6..0 is offset to alt_pwm_accum_charge_time,
//      byte[2] = bits 32..24 of value,
//      byte[3] = bits 23..16,
//      byte[4] = bits 15..8,
//      byte[5] = bits 7..0,
```

Battery absorption is somewhat indicated by alt_pwm_accum_charge_time, it accumulates milliseconds while alternat power is charging the battery (not the off-time). Four bytes from I2C have the value.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 18,0
{"txBuffer[2]":[{"data":"0x12"},{"data":"0x0"}]}
/1/ibuff 0,0,0,0
{"txBuffer[6]":[{"data":"0x12"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 6
{"rxBuffer":[{"data":"0x12"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
``` 

This is not yet working.


## Cmd 19 from a controller /w i2c-debug to set daynight manager i2c callbacks (4 x uint8_t).

Send four bytes to enable i2c callback for: daynight_state event, day event, night event. 

Address (0x31) is the i2c slave address that will receive the events. The daynight_state event (0x1) is the command byte used when daynight_state changes. The day (0x2) and night (0x3) event command is called so user functions can run functions.  The i2c slave needs to be implemented for the callbacks to operate, the manager will keep trying to master the i2c bus, but quit after an address NAK. 

``` 
# I am using the bootload interface 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 19,49,1,2,3
{"txBuffer[5]":[{"data":"0x13"},{"data":"0x31"},{"data":"0x1"},{"data":"0x2"},{"data":"0x3"}]}
/1/iread? 5
tbd
```

States defined in application need to match with the managers daynight_state.h file.

```
typedef enum DAYNIGHT_STATE_enum {
    DAYNIGHT_STATE_START, // Start day-night state machine
    DAYNIGHT_STATE_DAY, // day
    DAYNIGHT_STATE_EVENING_DEBOUNCE, // was day, maybe a dark cloud, or the PV panel got covered
    DAYNIGHT_STATE_NIGHTWORK, // task to at start of night, lights enabled, PV panel blocked so it does not drain battery
    DAYNIGHT_STATE_NIGHT, // night
    DAYNIGHT_STATE_MORNING_DEBOUNCE, // was night, maybe a flash light or...
    DAYNIGHT_STATE_DAYWORK, // task to at start of day, charge battery, water the garden
    DAYNIGHT_STATE_FAIL
} DAYNIGHT_STATE_t;
```


## Cmd 20 from a controller /w i2c-debug to access daynight manager uint16 values.

daynight_morning_threshold is used to check light on solar pannel e.g., ALT_V > this. Readings are taken when !ALT_EN.

daynight_evening_threshold is used to check light on solar pannel e.g., ALT_V < this. Readings are taken when !ALT_EN.

``` C
// I2C command to access daynight manager uint16 values.
// e.g., daynight_[morning_threshold|evening_threshold]
// I2C: byte[0] = 20, 
//      byte[1] = bit 7 is read/write 
//                bits 6..0 is offset to daynight_[morning_threshold|evening_threshold],
//      byte[2] = bits 15..8,
//      byte[3] = bits 7..0
``` 

To get the morning_threshold value.

``` 
# I am using the bootload interface 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 20,0,0,0
{"txBuffer[4]":[{"data":"0x14"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 4
{"rxBuffer":[{"data":"0x14"},{"data":"0x0"},{"data":"0x0"},{"data":"0x50"}]}
```

There are two ranges (12V and 24V) for solar panels; data that is outside the valid ranges is ignored. The settings are in daynight_limits.h. 

``` 
/1/ibuff 20,128,0,81
{"txBuffer[4]":[{"data":"0x14"},{"data":"0x80"},{"data":"0x0"},{"data":"0x51"}]}
/1/iread? 4
{"rxBuffer":[{"data":"0x14"},{"data":"0x80"},{"data":"0x0"},{"data":"0x50"}]}
/1/ibuff 20,0,0,0
{"txBuffer[4]":[{"data":"0x14"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 4
{"rxBuffer":[{"data":"0x14"},{"data":"0x0"},{"data":"0x0"},{"data":"0x51"}]}
```

We set the write bit in the first exchange, where the data sent is swapped with the default (80). The second exchange the updated value (81).


## Cmd 21 from the application controller /w i2c-debug to access daynight manager uint32 values.

0. daynight_morning_debounce is time that ALT_V > daynight_morning_threshold to change to DAY state.
1. daynight_evening_debounce is time that ALT_V < daynight_evening_threshold to change to NIGHT state.
2. elapsed(daynight_timer) which was time recorded at start of last daynight_state event.
3. elapsed(daynight_timer_at_night) which was time recorded at night work event.
4. elapsed(daynight_timer_at_day) which was time recorded at day work event
5. accumulate_alt_mega_ti_at_night is accumulated timed ALT_I readings (amp hour) recorded at night work event
6. accumulate_pwr_mega_ti_at_night is accumulated timed PWR_I readings (amp hour) recorded at night work event
7. accumulate_alt_mega_ti_at_day is accumulated timed ALT_I readings (amp hour) recorded at day work event
8. accumulate_pwr_mega_ti_at_day is accumulated timed PWR_I readings (amp hour) recorded at day work event

``` C
// I2C command to access daynight manager uint32 values
// e.g., daynight_[morning_debounce|evening_debounce],
//       elapsed daynight_[timer|timer_at_night|timer_at_day],
//       Amp Hr accumulate_[alt_mega_ti_at_night|pwr_mega_ti_at_night|alt_mega_ti_at_day|pwr_mega_ti_at_day]
// I2C: byte[0] = 21, 
//      byte[1] = bit 7 is read/write 
//                bits 6..0 is offset to daynight uint32 value,
//      byte[2] = bits 32..24 of uint32 value,
//      byte[3] = bits 23..16,
//      byte[4] = bits 15..8,
//      byte[5] = bits 7..0,
```

Read the daynight_evening_debounce value. Four bytes from I2C have the value.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 21,1
{"txBuffer[2]":[{"data":"0x15"},{"data":"0x1"}]}
/1/ibuff 0,0,0,0
{"txBuffer[6]":[{"data":"0x15"},{"data":"0x1"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 6
{"rxBuffer":[{"data":"0x15"},{"data":"0x1"},{"data":"0x0"},{"data":"0x12"},{"data":"0x4F"},{"data":"0x80"}]}
``` 

The four bytes sum to 1,200,000 (e.g., 0*(2**24) + 0x12*(2**16) + 0x4F*(2**8) + 0x80) mSec or 20 min. Send command and an ignored long integer (0) in four bytes to see what the evening_debouce value is.



Values that are outside the valid range are ignored (8000 to 3,600,000 or 8sec to 60 min). The limits are in daynight_limits.h. 

``` 
/1/ibuff 21,129
{"txBuffer[2]":[{"data":"0x15"},{"data":"0x81"}]}
/1/ibuff 0,0,31,65
{"txBuffer[6]":[{"data":"0x15"},{"data":"0x81"},{"data":"0x0"},{"data":"0x0"},{"data":"0x1F"},{"data":"0x41"}]}
/1/iread? 6
{"rxBuffer":[{"data":"0x15"},{"data":"0x81"},{"data":"0x0"},{"data":"0x12"},{"data":"0x4F"},{"data":"0x80"}]}
/1/ibuff 21,1
{"txBuffer[2]":[{"data":"0x15"},{"data":"0x1"}]}
/1/ibuff 0,0,0,0
{"txBuffer[6]":[{"data":"0x15"},{"data":"0x1"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 6
{"rxBuffer":[{"data":"0x15"},{"data":"0x1"},{"data":"0x0"},{"data":"0x0"},{"data":"0x1F"},{"data":"0x41"}]}
```

The value 8001 (e.g., 31*(2**8) + 65) sent was swapped with the default (20 min); the second exchange has the updated value (8 sec).

The Amp-Hour values needs documentation to show how to convert them. 


## Cmd 22 is not used.

repurpose


## Cmd 23 is not used.

repurpose
