# BlinkLED

## Overview

Blink the manager status LED plus use the host I2C interface (e.g. the secnond I2C port on ATmega328pb).


## Firmware Upload

Use an ICSP tool connected to the bus manager (set the ISP_PORT in Makefile) run 'make isp' and it should compile and then flash the bus manager.

```
sudo apt-get install make git gcc-avr binutils-avr gdb-avr avr-libc avrdude
git clone https://github.com/epccs/Gravimetric/
cd /Gravimetric/Manager/BlinkLED
make all
make isp
...
avrdude done.  Thank you.
```

## How To Use

The [i2c1-debug] firmware can be used to test the manager's SMBus interface (twi1) at slave address 0x2A from the controller's I2C user interface (twi1). The application controler and manager use there other interface (twi0) as a private I2C bus. 

[i2c1-debug]: https://github.com/epccs/Gravimetric/tree/master/Applications/i2c1-debug

```
picocom -b 38400 /dev/ttyUSB0
...
Terminal ready
/0/i1scan?
{"scan":[{"addr":"0x2A"}]}
/0/i1addr 42
{"master_address":"0x2A"}
/0/i1buff 0,3
{"txBuffer[2]":[{"data":"0x0"},{"data":"0x3"}]}
/0/i1write
{"txBuffer":"wrt_success"}
#blinking has stopped
/0/i1buff 0
{"txBuffer[2]":[{"data":"0x0"}]}
/0/i1read? 2
{"txBuffer":"wrt_success","rxBuffer":"rd_success","rxBuffer":[{"data":"0x0"},{"data":"0xFC"}]}
/0/i1buff 0,15
{"txBuffer[2]":[{"data":"0x0"},{"data":"0xF"}]}
/0/i1write
{"txBuffer":"wrt_success"}
#blinking again
```

SMBus: After write_i2c_block_data the 328pb holds the old data. When read_i2c_block_data occures it will put a I2C packet with the address followed by a request to read (which is the active transaction), at which point the slave returns the data for the previous write (e.g. the previous transaction). Now have a look at the [Python program] for a Raspery Pi.

[Python program]: https://github.com/epccs/Gravimetric/tree/master/Manager/BlinkLED/toggle.py
