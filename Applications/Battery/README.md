# Battery Charging

##ToDo

```
(done) pwm mode high time/low time needs to be locked at the start of a period, it has problems otherwise.
(done) add cli to change battery_low_limit and battery_high_limit
(done) batmgr_state name change to bm_state
(done) add a battery_host_limit to halt the host and turn off its power.
alt_pwm_accum_charge_time is not accumulating.
change the low and high limit to be the PWM range
```


 Overview

The manager has control of the Alternate power input that may be used to send current from a charging supply to the primary power input (with the battery). 

The alternate power supply needs to act as a current source, and the principal power source needs to act as a battery. Do not attempt this with a bench power supply as the primary power input, since many will not tolerate back drive. When the alternate power is enabled, it must have a current limit bellow what the parts and battery can handle.

A DayNight state machine is on the manager; it has two work events. When the DayNight state shows it is day state the enabled battery state machine controls the alternate power input. Analog measurements occur every ten milliseconds. The primary power measurement channel (PWR_V) is used to measure the battery voltage with a voltage divider (1% 15.8k and 0.1% 100k). The battery management state machine controls the alternate input depending on the measured battery voltage in relation to battery_low_limit and battery_high_limit. The alternate input will PWM when the battery is above midway between the battery_low_limit and battery_high_limit, and operate as constant current bellow the midpoint. 


## Wiring Needed

Same as DayNight

![Wiring](../DayNight/Setup/AuxilaryWiring.png)


# Firmware Upload

The manager needs to be told that it has a localhost and the address to bootload. I do this with some commands on the SMBus interface from an R-Pi.

``` 
sudo apt-get install i2c-tools python3-smbus
sudo usermod -a -G i2c your_user_name
# logout for the change to take
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# what is my address
bus.write_i2c_block_data(42, 0, [255])
print("'"+chr(bus.read_i2c_block_data(42, 0, 2)[1])+"'" )
'2'
# I want to bootload address '1'
bus.write_i2c_block_data(42, 3, [ord('1')])
print("'"+chr(bus.read_i2c_block_data(42, 3, 2)[1])+"'" )
'1'
# clear the host lockout status bit so nRTS from my R-Pi on '2' will triger the bootloader on '1'
bus.write_i2c_block_data(42, 7, [0])
print(bus.read_i2c_block_data(42,7, 2))
[7, 0]
exit()
```

Or use the bootload port.

Now the serial port connection (see BOOTLOAD_PORT in Makefile) can reset the MCU and execute optiboot so that the 'make bootload' rule can upload a new binary image in the application area of flash memory.

``` 
sudo apt-get install make git picocom gcc-avr binutils-avr gdb-avr avr-libc avrdude
git clone https://github.com/epccs/Gravimetric/
cd /Gravimetric/Applications/Battery
make
make bootload
...
avrdude done.  Thank you.
``` 

Now connect with picocom (or ilk).


``` 
#exit is C-a, C-x
picocom -b 38400 /dev/ttyAMA0
# with bootload port
picocom -b 38400 /dev/ttyUSB0
``` 

# Commands

Commands are interactive over the serial interface at 38400 baud rate. The echo will start after the second character of a new line. 


## /\[rpu_address\]/\[command \[arg\]\]

rpu_address is taken from the I2C address 0x29 (e.g. get_Rpu_address() which is included form ../lib/rpu_mgr.h). The value of rpu_address is used as a character in a string, which means don't use a null value (C strings are null terminated) as an adddress. The ASCII value for '1' (0x31) is easy and looks nice, though I fear it will cause some confusion when it is discovered that the actual address value is 49.

Commands and their arguments follow.


## /0/id? \[name|desc|avr-gcc\]

identify 

``` 
/1/id?
{"id":{"name":"Battery","desc":"Gravimetric (17341^1) Board /w ATmega324pb","avr-gcc":"5.4.0"}}
```

##  /0/bmcntl?

Reports battery manager control values. 

``` 
/1/bmcntl?
{"bm_state":"0x0","bat_low_lim":"374","bat_high_lim":"397","bat_host_lim":"307","adc_pwr_v":"387","adc_alt_v":"0","pwm_timer":"0","dn_timer":"13810335"}
{"bm_state":"0x0","bat_low_lim":"374","bat_high_lim":"397","bat_host_lim":"307","adc_pwr_v":"386","adc_alt_v":"0","pwm_timer":"0","dn_timer":"13815335"}
{"bm_state":"0x0","bat_low_lim":"374","bat_high_lim":"397","bat_host_lim":"307","adc_pwr_v":"387","adc_alt_v":"0","pwm_timer":"0","dn_timer":"13820334"}
``` 

The non-calibrated default battery_low_limit is 374, and battery_high_limit is 398. The battery and alternat input have 12.8V during the above reading.

##  /0/bm

This will togle the battery manager enable

``` 
/1/bm
{"bat_en":"ON"}
```

##  /0/bmlowlim? \[0..1023\]

This will set the battery manager low limit used with 10 bit ADC.

``` 
/1/bmlow?
{"bat_low_lim":"374"}
/1/bmlow? 373
{"bat_low_lim":"373"}
```

##  /0/bmhighlim? \[0..1023\]

This will set the battery manager high limit used with 10 bit ADC.

``` 
/1/bmhigh?
{"bat_high_lim":"398"}
/1/bmhigh? 399
{"bat_high_lim":"399"}
```


##  /0/bmhost? \[0..1023\]

This will set the battery manager host limit used with 10 bit ADC to shutdown the host when the battery is bellow.

``` 
/1/bmhost?
{"bat_host_lim":"307"}
/1/bmhost? 308
{"bat_host_lim":"308"}
```
