# Analog-to-Digital Converter

## Overview

Adc is an interactive command line program that demonstrates control of an Analog-to-Digital Converter. 

A customized library routine is used to operate the AVR's ADC, it has an ISR that is started with the enable_ADC_auto_conversion function to read the channels one after the next in a burst (free running is also an option). In this case, the loop starts the burst at timed intervals. The ADC clock runs at 125kHz and it takes 25 of its clocks to do the analog conversion, thus a burst takes over (ISR overhead) 1.6 milliseconds (e.g. 8*25*(1/125000)) to scan the eight channels. The ADC is turned off after each burst.

The ADMUX register is used when selecting ADC channels. 


# EEPROM Memory map 

Below is a map of calibration values placed in EEPROM during [SelfTest]. 

```
function            type        ee_addr:
id                  UINT16      30
ref_extern_avcc     UINT32      32
ref_intern_1v1      UINT32      36
```

The AVCC pin is used to power the analog to digital converter and is also selected as a reference. On Gravimetric, the AVCC pin is powered by a switchmode supply through a filter that can be measured and then used as a calibrated reference.

The internal 1V1 bandgap is not trimmed by the manufacturer, so it needs to be measured. However, once it is known, it is a useful reference.

[SelfTest] loads calibration values for references in EEPROM.

[SelfTest]: https://github.com/epccs/Gravimetric/tree/master/Applications/SelfTest


# Manager ADC and Callibration Values

The application controller and manager have a private I2C bus between them.

```
Callibraion         type        i2c command/select/data         manager defaults 
alt_i               INT16       32,0,0                          32,(10bits from adc ch 0)
alt_v               INT16       32,0,1                          32,(10bits from adc ch 1)
pwr_i               INT16       32,0,2                          32,(10bits from adc ch 6)
pwr_v               INT16       32,0,3                          32,(10bits from adc ch 7)
ref_extern_avcc     FLOAT       38,0,0,0,0,0                    38,0,0x40,0xA0,0x0,0x0
ref_intern_1v1      FLOAT       38,1,0,0,0,0                    38,1,0x3F,0x8A,0x3D,0x71
alt_i_callibraion   FLOAT       33,0,0,0,0,0                    33,0,0x3A,0x8E,0x38,0xE4
alt_v_callibraion   FLOAT       33,1,0,0,0,0                    33,1,0x3C,0x30,0x0,0x0
pwr_i_callibraion   FLOAT       33,2,0x39,0x96,0x96,0x96        33,2,0x39,0x96,0x96,0x96
pwr_v_callibraion   FLOAT       33,3,0x3B,0xEA,0x88,0x1A        33,3,0x3B,0xEA,0x88,0x1A
```


# Firmware Upload

The manager connected to the host that uploads needs to be advised what bootload address will receive the upload. I do this with some commands on the SMBus interface from a Raspberry Pi single-board computer.

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

Now the serial port connection (see BOOTLOAD_PORT in Makefile) can reset the MCU and execute the receiving bootloader (optiboot) so that the 'make bootload' rule can upload a new binary image in the application area of the MCU's flash memory.

``` 
sudo apt-get install make git picocom gcc-avr binutils-avr gdb-avr avr-libc avrdude
git clone https://github.com/epccs/Gravimetric/
cd /Gravimetric/Applications/Adc
make
make bootload
...
avrdude done.  Thank you.
``` 

Now connect with picocom (or ilk).


``` 
#exit is C-a, C-x
picocom -b 38400 /dev/ttyAMA0
``` 

or log the terminal session

``` 
script -f -c "picocom -b 38400 /dev/ttyUSB0" stuff.log
``` 


# Commands

Commands are interactive over the serial interface at 38400 baud rate. The echo will start after the second character of a new line. 


## /\[rpu_address\]/\[command \[arg\]\]

rpu_address is taken from the manager (e.g. get_Rpu_address() which is included form ../lib/rpu_mgr.h). The value of rpu_address is used as a character in a string, which means don't use a null value (C strings are null terminated) as an adddress. The ASCII value for '1' (0x31 or 49) is easy and looks nice.

Commands and their arguments follow.


## /0/id? \[name|desc|avr-gcc\]

identify 

``` 
/1/id?
{"id":{"name":"Adc","desc":"Gravimetric (17341^1) Board /w ATmega324pb","avr-gcc":"5.4.0"}}
```

##  /0/analog? 0..7\[,0..7\[,0..7\[,0..7\[,0..7\]\]\]\]    

Analog-to-Digital Converter reading from up to 5 channels. The reading repeats every 2 Seconds until the Rx buffer gets a character. Channel 11 is the input voltage (PWR_V), channel 10 is the input current (PWR_I), channel 8 is the alternate input current (ALT_I), channel 9 is the alternate input voltage (ALT_V), channels 0..3 can read up to about 4.5V (higher voltages are blocked by a level shift). Channel 4..7 are from floating test points. Channels 8..11 are from the manager over a private I2C connection. 

``` 
/1/analog? 8,9,10,11
{"ALT_I":"0.00","ALT_V":"0.00","PWR_I":"0.02","PWR_V":"12.81"}
/1/analog? 0,1,2,3,4
{"ADC0":"3.75","ADC1":"3.77","ADC2":"3.77","ADC3":"3.77","ADC4":"3.77"}
```

Channels 0..4 were floating when I ran the above.

Channels 8..11 are taken from the manager over the I2C interface.


##  /0/avcc 4500000..5500000

Calibrate the AVCC reference in microvolts.

``` 
# do this with the SelfTest
/1/avcc 4958500
{"REF":{"extern_avcc":"4.9585",}}
``` 


##  /0/onevone 900000..1300000

Set the 1V1 internal bandgap reference in microvolts.

```
# do this with the SelfTest
/1/onevone 1100000
{"REF":{"intern_1v1":"1.1000",}}
``` 

The SelfTest will calculate this value when it is ran based on the AVCC value compiled into source.


##  /0/reftoee

Save the reference in static memory to EEPROM.

```
# do this with the SelfTest
/1/reftoee
{"REF":{"extern_avcc":"4.9585","intern_1v1":"1.1000",}}
```


##  /0/reffrmee

Load the reference from EEPROM into static memory.

```
/1/reffrmee
{"REF":{"extern_avcc":"4.9785","intern_1v1":"1.0858",}}
```

