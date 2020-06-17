# PV, DayNight and Battery Management Commands

16..31 (Ox10..0x1F | 0b00010000..0b00011111)

16. Battery manager, enable with callback address (i2c), and and comand number to send state callback value to.
17. Access battery manager uint16 values.
18. not used
19. not used
20. Battery absorption (e.g., alt_pwm_accum_charge_time) time (uint32_t)
21. morning_threshold (int16_t). Day starts when ALT_V is above morning_threshold for morning_debouce time.
22. evening_threshold (int16_t). Night starts when ALT_V is bellow evening_threshold for evening_debouce time.
23. Day-Night i2c callback (callback address, report state cmd, day event cmd, night event cmd).

## Cmd 16 from a controller /w i2c-debug to enable battery manager

Enable the battery manager. The first byte is a callback address (e.g., the slave address to use). The second byte is used as a command number to receive the battery manager state machine events that are sent to the application. The last byte is used to enable = 1..255 / disable = 0 the battery manager. 

``` 
# I am using the bootload interface 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 16,49,0,1
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


## Cmd 18 is not used.

repurpose


## Cmd 19 is not used.

repurpose


## Cmd 20 from the application controller /w i2c-debug running read alt_pwm_accum_charge_time.

Battery absorption occures durring alt_pwm_accum_charge_time, it accumulates the millis time while alternat power was used to charge the battery not the off time. Read four bytes from I2C. They are bytes to a UINT32 from a buffered accumulation of millis reading.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 20,0,0,0,0
{"txBuffer[5]":[{"data":"0x14"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 5
{"rxBuffer":[{"data":"0x14"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
``` 

This needs checked with a battery.


## Cmd 21 from a controller /w i2c-debug to access morning_threshold

Send an ignored integer (0) in two bytes to see what the morning_threshold value is.

``` 
# I am using the bootload interface 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 21,0,0
{"txBuffer[3]":[{"data":"0x15"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 3
{"rxBuffer":[{"data":"0x15"},{"data":"0x0"},{"data":"0x50"}]}
```

There are two ranges (12V and 24V) for solar panels; data that is outside the valid area is ignored. The settings are in daynight_limits.h. 

``` 
/1/ibuff 21,0,81
{"txBuffer[3]":[{"data":"0x15"},{"data":"0x0"},{"data":"0x51"}]}
/1/iread? 3
{"rxBuffer":[{"data":"0x15"},{"data":"0x0"},{"data":"0x50"}]}
/1/ibuff 21,0,0
{"txBuffer[3]":[{"data":"0x15"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 3
{"rxBuffer":[{"data":"0x15"},{"data":"0x0"},{"data":"0x51"}]}
```

The data sent was swapped with the default (80); the second exchange has ignored data that is swapped with the updated value (81).


## Cmd 22 from a controller /w i2c-debug to access evening_threshold

Send an ignored integer (0) in two bytes to see what the evening_threshold value is.

``` 
# I am using the bootload interface 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 22,0,0
{"txBuffer[3]":[{"data":"0x16"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 3
{"rxBuffer":[{"data":"0x16"},{"data":"0x0"},{"data":"0x28"}]}
```

There are two ranges (12V and 24V) for solar panels; data that is outside the valid area is ignored. The settings are in daynight_limits.h. 

``` 
/1/ibuff 22,0,37
{"txBuffer[3]":[{"data":"0x16"},{"data":"0x0"},{"data":"0x25"}]}
/1/iread? 3
{"rxBuffer":[{"data":"0x16"},{"data":"0x0"},{"data":"0x28"}]}
/1/ibuff 22,0,0
{"txBuffer[3]":[{"data":"0x16"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 3
{"rxBuffer":[{"data":"0x16"},{"data":"0x0"},{"data":"0x25"}]}
```

The data sent was swapped with the default (40); the second exchange has ignored data that is swapped with the updated value (37).


## Cmd 23 from a controller /w i2c-debug to set Day-Night i2c callback (4 x uint8_t).

Send four bytes to enable i2c callback for events. Address, daynight_state events, day event, night event. 

Address (0x31) is the i2c slave address that will receive the events. The daynight_state event (0x1) is the command byte used when daynight_state changes. The day (0x2) and night (0x3) event command is called so user functions can run functions.  The i2c slave needs to be implemented for the callbacks to operate, the manager will keep trying to master the i2c bus, but quit after an address NAK. 

``` 
# I am using the bootload interface 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 23,43,1,2,3
{"txBuffer[5]":[{"data":"0x17"},{"data":"0x31"},{"data":"0x1"},{"data":"0x2"},{"data":"0x3"}]}
/1/iread? 5

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