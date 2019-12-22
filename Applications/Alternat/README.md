# Alternate Power Input

ToDo: this is WIP, the manager needs to expose more of how the charger works so I can report it with this app.


## Overview

The manager has control of the Alternate power input that may be used to send current from the alternate supply to the primary power input. 

The alternate power needs to act as a current source, and the principal power source needs to act as a battery. Do not attempt this with a bench power supply as the primary power input, since many will not tolerate back drive. When the alternate power is enabled, it must have a current limit bellow a safe level at which the battery can charge.

The DayNight state machine is on the manager; it has two work states. The day work state is to enable the alternate input, while the night work state turns it off. Analog measurements are from the ADC during a burst of interrupts every ten milliseconds. The ADC mux channel code-named PWR_V is connected to an input voltage divider (a 1% 15.8k and a 0.1% 100k) and used to measure the battery voltage. The manager will enable the Alternate input when the battery is bellow battery_low_limit, and enable_alternate_power has been established (for details see the power_manager.c file on the manager).


## Wiring Needed

Same as DayNight

![Wiring](../DayNight/Setup/AuxilaryWiring.png)


# Firmware Upload

With a serial port connection (see BOOTLOAD_PORT in Makefile) and optiboot installed on the RPUno run 'make bootload' and it should compile and then flash the MCU.

``` 
sudo apt-get install make git gcc-avr binutils-avr gdb-avr avr-libc avrdude
git clone https://github.com/epccs/RPUno/
cd /RPUno/Alternat
make bootload
...
avrdude done.  Thank you.
``` 

Now connect with picocom (or ilk).

``` 
#exit is C-a, C-x
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
{"id":{"name":"Alternat","desc":"Gravimetric (17341^1) Board /w ATmega324pb","avr-gcc":"5.4.0"}}
```

##  /0/alt

This will report alternat enable

``` 
/1/alt
{"alt_en":"OFF"}
```

##  /0/altcntl?

Reports alternat power control values. 

``` 
/1/altcntl?
{"mgr_alt_en":"0x0","charge_start":"374","charge_stop":"398"}
``` 

The non calibrated default charge_start is 374, and charge_stop is 398. 
