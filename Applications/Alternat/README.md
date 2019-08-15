# Alternate Power Input

## Overview

Alternate power input is used to send current from a solar panel into a battery. 

The alternate power source needs to act as a current source, and the main power source needs to act like a battery. Do not attempt this with a bench power supply as the main power input. When the alternate power is enabled it must current limit and then charge the battery.

The DayNight state machine is used, it has two events that run a registered callback function. The day event allows the alternate power input to enable, while the night event keeps it off. Adc channels are measured with a burst of interrupts that occurs every 100 millis. Near the time of the Adc burst, the mux channel PWR_V which is connected to an input voltage divider (a 1% 15.8k and a 0.1% 100k) is checked. This is more or less the battery voltage so can be used to turn off the Alternate power input when the battery has reached a max voltage, and then turn it back on when the voltage drops.

The hardware for this lacks training wheels, it is going to take some time to refine these ideas. The target battery is lead acid, this method fails with other types. 


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
{"id":{"name":"Alternat","desc":"Gravimetric (17341^0) Board /w ATmega324pb","avr-gcc":"5.4.0"}}
```

##  /0/alt

This will toggle the Alternat Power enable

``` 
/1/alt
{"alt":"ON"}
```

##  /0/altcnt?

This gives the number of times the charging limit was reached, it is a clue about how charged the battery is.

``` 
/1/altcnt?
{"alt_count":"2"}
``` 

Example with limit set at 13.6V and 180mA CC supply on ALT and 18AHr AGM battery on PWR. 

``` 
/1/alt
{"alt_en":"ON"}
/1/altcnt?
{"alt_count":"0"}
/1/analog? 4,5,6,7
{"ALT_V":"12.94","ALT_I":"0.075","PWR_I":"0.048","PWR_V":"12.92"}
{"ALT_V":"13.00","ALT_I":"0.075","PWR_I":"0.048","PWR_V":"12.95"}
{"ALT_V":"13.00","ALT_I":"0.075","PWR_I":"0.048","PWR_V":"12.95"}
{"ALT_V":"13.16","ALT_I":"0.070","PWR_I":"0.048","PWR_V":"13.20"}
{"ALT_V":"13.32","ALT_I":"0.075","PWR_I":"0.048","PWR_V":"13.34"}
{"ALT_V":"13.48","ALT_I":"0.070","PWR_I":"0.047","PWR_V":"13.49"}
{"ALT_V":"14.17","ALT_I":"0.005","PWR_I":"0.048","PWR_V":"13.56"}
/1/altcnt?
{"alt_count":"1"}
{"ALT_V":"13.48","ALT_I":"0.075","PWR_I":"0.048","PWR_V":"13.45"}
{"ALT_V":"13.42","ALT_I":"0.075","PWR_I":"0.048","PWR_V":"13.49"}
{"ALT_V":"13.48","ALT_I":"0.075","PWR_I":"0.048","PWR_V":"13.49"}
{"ALT_V":"13.53","ALT_I":"0.075","PWR_I":"0.048","PWR_V":"13.49"}
{"ALT_V":"13.53","ALT_I":"0.070","PWR_I":"0.048","PWR_V":"13.52"}
{"ALT_V":"13.53","ALT_I":"0.075","PWR_I":"0.048","PWR_V":"13.52"}
{"ALT_V":"13.58","ALT_I":"0.075","PWR_I":"0.048","PWR_V":"13.56"}
{"ALT_V":"13.58","ALT_I":"0.075","PWR_I":"0.048","PWR_V":"13.59"}
{"ALT_V":"13.53","ALT_I":"0.075","PWR_I":"0.048","PWR_V":"13.56"}
{"ALT_V":"14.17","ALT_I":"0.005","PWR_I":"0.028","PWR_V":"13.52"}
{"ALT_V":"13.64","ALT_I":"0.178","PWR_I":"0.030","PWR_V":"13.59"}
{"ALT_V":"14.17","ALT_I":"0.000","PWR_I":"0.028","PWR_V":"13.56"}
{"ALT_V":"13.53","ALT_I":"0.167","PWR_I":"0.030","PWR_V":"13.52"}
{"ALT_V":"14.17","ALT_I":"0.000","PWR_I":"0.027","PWR_V":"13.52"}
{"ALT_V":"14.17","ALT_I":"0.005","PWR_I":"0.030","PWR_V":"13.45"}
{"ALT_V":"13.58","ALT_I":"0.172","PWR_I":"0.030","PWR_V":"13.59"}
{"ALT_V":"13.58","ALT_I":"0.178","PWR_I":"0.030","PWR_V":"13.63"}
/1/altcnt?
{"alt_count":"5"}
```

Note: ALT_I was changed from 90mA to 180mA when looking at this. 

## [/0/analog? 0..7\[,0..7\[,0..7\[,0..7\[,0..7\]\]\]\]](../Adc#0analog-0707070707)
