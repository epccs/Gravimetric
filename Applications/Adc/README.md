# Analog-to-Digital Converter

## Overview

Adc is an interactive command line program that demonstrates control of an Analog-to-Digital Converter. 

A customized library routine is used to operate the AVR's ADC, it has an ISR that is started with the enable_ADC_auto_conversion function to read the channels one after the next in a burst (free running is also an option). In this case, the loop starts the burst at timed intervals. The ADC clock runs at 125kHz and it takes 25 of its clocks to do the analog conversion, thus a burst takes over (ISR overhead) 1.6 milliseconds (e.g. 8*25*(1/125000)) to scan the eight channels. The ADC is turned off after each burst.

The ADMUX register is used when selecting ADC channels. On this board the analog channel matches the digital number used by the [Wiring] functions. Thus the pin with ADC channel 0 (e.g., analogRead(0)) also has Wiring number 0 for digital (e.g. pinMode(0,INPUT), digitalRead(0) ) functions.

[Wiring]: https://arduinohistory.github.io/


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

Analog-to-Digital Converter reading from up to 5 ADMUX channels. The reading repeats every 2 Seconds until the Rx buffer gets a character. Channel 7 is the input voltage (PWR_V), channel 6 is the input current (PWR_I), channel 5 is the alternat input current (ALT_I), channel 4 is the alternat input voltage (ALT_V), channel 3, 2,  1, and 0 inputs can read up to about 4.5V (higher voltages are blocked by a level shift).

``` 
/1/analog? 4,5,6,7
{"ALT_V":"0.00","ALT_I":"0.000","PWR_I":"0.009","PWR_V":"12.79"}
/1/analog? 0,1,2,3
{"ADC0":"0.00","ADC1":"0.03","ADC2":"0.00","ADC3":"0.00"}
```

Channels 0..3 were connected to the [SelfTest] setup when I ran the above.

Channels 4..7 are taken from the manager over the I2C interface (on ^0 of this board they were on these respective channels).


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

