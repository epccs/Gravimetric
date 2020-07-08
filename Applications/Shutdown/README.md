# Host Shutdown

##ToDo

```
(done) /hs does not UP the HOST, call EnableShutdownCntl not EnableBatMngCntl.
(done) sd_state is stuck on CURR_CHK (sd_state = 3) when going DOWN. Was loading uninitialized EEPROM into the values.
(done) hs_timer was not available from manager.
(done) set up i2c callback poke for shutdown (byte[3] = bring host UP[1], take host DOWN[0], poke[2..254].)
```


 Overview

The manager has control of the power to the Raspberry Pi Single-Board Computer (SBC). A process of checking that the R-Pi has halt before removing power is managed by, you guessed it, the manager.

The Raspberry Pi single-board computer needs to run a shutdown or halt script that monitors BCM6. The shutdown can take some time as it attempts to kill processes and ready the system for entering a halt. After processes stop and files are flushed, the halt is detected with a reduced current usage on the power input that the manager has access to monitor.  Since the SD may have had significant activity durring file flushing we need to wait and then verify system curr usage is stable before the final act of removing power from the R-Pi/SBC.


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
cd /Gravimetric/Applications/Shutdown
make
make bootload
...
avrdude done.  Thank you.
``` 

Now connect with picocom (or ilk).


``` 
#exit is C-a, C-x
# from an R-Pi
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
{"id":{"name":"Shutdown","desc":"Gravimetric (17341^1) Board /w ATmega324pb","avr-gcc":"5.4.0"}}
```


##  /0/hs

This will toggle the host shutdown control.

``` 
/1/hs
{"hs_en":"UP"}
```

Its report is based on toggling the hs_state that was poked by way of callback from the manager after setup, e.g. after an application reset. 


##  /0/hscntl?

Report host shutdown control values. 

``` 
/1/hscntl?
{"sd_state":"0x9","hs_halt_curr":"63","adc_pwr_i":"35","hs_ttl":"60000","hs_delay":"10000","hs_wearlv":"100","hs_timer":"0"}
{"sd_state":"0x9","hs_halt_curr":"63","adc_pwr_i":"26","hs_ttl":"60000","hs_delay":"10000","hs_wearlv":"100","hs_timer":"1"}
/1/hs
{"hs_en":"UP"}
/1/hscntl?
{"sd_state":"0xB","hs_halt_curr":"63","adc_pwr_i":"37","hs_ttl":"60000","hs_delay":"10000","hs_wearlv":"100","hs_timer":"10983"}
{"sd_state":"0xB","hs_halt_curr":"63","adc_pwr_i":"23","hs_ttl":"60000","hs_delay":"10000","hs_wearlv":"100","hs_timer":"15982"}
...
{"sd_state":"0xB","hs_halt_curr":"63","adc_pwr_i":"24","hs_ttl":"60000","hs_delay":"10000","hs_wearlv":"100","hs_timer":"55972"}
{"sd_state":"0x0","hs_halt_curr":"63","adc_pwr_i":"36","hs_ttl":"60000","hs_delay":"10000","hs_wearlv":"100","hs_timer":"0"}
/1/hs
{"hs_en":"DOWN"}
/1/hscntl?
{"sd_state":"0x6","hs_halt_curr":"63","adc_pwr_i":"23","hs_ttl":"60000","hs_delay":"10000","hs_wearlv":"100","hs_timer":"1229"}
{"sd_state":"0x6","hs_halt_curr":"63","adc_pwr_i":"38","hs_ttl":"60000","hs_delay":"10000","hs_wearlv":"100","hs_timer":"6229"}
{"sd_state":"0x9","hs_halt_curr":"63","adc_pwr_i":"22","hs_ttl":"60000","hs_delay":"10000","hs_wearlv":"100","hs_timer":"0"}
{"sd_state":"0x9","hs_halt_curr":"63","adc_pwr_i":"38","hs_ttl":"60000","hs_delay":"10000","hs_wearlv":"100","hs_timer":"0"}
...
``` 

sd_state: 0x0 is UP, 0x6 is DELAY, 0x9 is DOWN, 0xB is RESTART_DLY.


##  /0/hshaltcurr? \[0..1023\]

This will access shutdown_halt_curr_limit on manager. Befor host shutdown is done PWR_I current must be bellow this limit.

``` 
/1/hshaltcurr?
{"hs_halt_curr":"63"}
/1/hshaltcurr? 64
{"hs_halt_curr":"64"}
```


##  /0/hsttl? \[uint32\]

This will access shutdown_ttl_limit on manager. Time to wait for PWR_I to be bellow shutdown_halt_curr_limit and then stable for wearleveling

``` 
/1/hsttl?
{"hs_ttl":"60000"}
/1/hsttl? 60001
{"hs_ttl":"60001"}
```


##  /0/hsdelay? \[uint32\]

This will access shutdown_delay_limit on manager. Time to wait after droping bellow shutdown_halt_curr_limit, but befor checking wearleveling for stable readings.

``` 
/1/hsdelay?
{"hs_delay":"10000"}
/1/hsdelay? 10001
{"hs_delay":"10001"}
```


##  /0/hswearlv? \[uint32\]

This will access shutdown_wearleveling_limit on manager. Time PWR_I must be stable for.

``` 
/1/hswearlv?
{"hs_wearlv":"100"}
/1/hswearlv? 101
{"hs_wearlv":"101"}
```

