# Manager

This firmware is for the manager on an RPUBUS board, it will watch for a byte matching the local RPU_ADDRESS on the DTR pair and when seen reset the local controller placing it in bootloader mode. A non-matching byte will disconnect the RX and TX lines from the local controller and be in lockout mode until a LOCKOUT_DELAY completes or an RPU_NORMAL_MODE byte is seen on the DTR pair.

## Overview

In normal mode, the RX and TX lines (twisted pairs) are connected through transceivers to the controller board RX and TX pins while the DTR pair is connected to the bus manager UART and is used to set the system-wide bus state.

During lockout mode, the RX and TX serial lines are disconnected at the transceivers from the controller board RX and TX pins. Use LOCKOUT_DELAY to set the time in mSec.

Bootload mode occurs when a byte on the DTR pair matches the RPU_ADDRESS of the shield. It will cause a pulse on the reset pin to activate the bootloader on the local microcontroller board. After the bootloader is done, the local microcontroller will run its application, which, if it reads the RPU_ADDRESS will cause the manager to send an RPU_NORMAL_MODE byte on the DTR pair. The RPU_ADDRESS is read with a command from the controller board. If the RPU_ADDRESS is not read, the bus manager will timeout but not connect the RX and TX transceivers to the local controller board. Use BOOTLOADER_ACTIVE to set the time in mSec; it needs to be less than LOCKOUT_DELAY.

The lockout mode occurs when a byte on the DTR pair does not match the RPU_ADDRESS of the manager. It will cause the lockout condition and last for a duration determined by the LOCKOUT_DELAY or when an RPU_NORMAL_MODE byte is seen on the DTR pair.

When nRTS (or nDTR on RPUadpt) are pulled active the bus manager will connect the HOST_TX and HOST_RX lines to the RX and TX pairs, and pull the nCTS (and nDSR) lines active to let the host know it is Ok to send. If the bus is in use, the host will remain disconnected from the bus. Note the Remote firmware sets a status bit at startup that prevents the host from connecting until it is cleared with an I2C command.

Arduino Mode is a permanent bootload mode so that the Arduino IDE can connect to a specific address (e.g., point to point). It needs to be enabled by I2C or SMBus. The command will cause a byte to be sent on the DTR pair that sets the arduino_mode thus overriding the lockout timeout and the bootload timeout (e.g., lockout and bootload mode are everlasting).

Test Mode. I2C command to swithch to test_mode (save trancever control values). I2C command to recover trancever control bits after test_mode.


## Firmware Upload

Use an ICSP tool connected to the bus manager (set the ISP_PORT in Makefile) run 'make' to compile then 'make isp' to flash the bus manager (Notes of fuse setting included).

```
sudo apt-get install make git gcc-avr binutils-avr gdb-avr avr-libc avrdude
git clone https://github.com/epccs/Gravimetric/
cd /Gravimetric/Manager/manager
make
make isp
...
avrdude -v -p atmega328pb -C +../lib/avrdude/328pb.conf -c stk500v1 -P /dev/ttyACM0 -b 19200 -U eeprom:r:manager_atmega328pb_eeprom.hex:i
...
avrdude: safemode: Fuses OK (E:F7, H:D9, L:62)
...
avrdude -v -p atmega328pb -C +../lib/avrdude/328pb.conf -c stk500v1 -P /dev/ttyACM0 -b 19200 -e -U lock:w:0xFF:m -U lfuse:w:0xFF:m -U hfuse:w:0xD6:m -U efuse:w:0xFD:m
...
avrdude: safemode: Fuses OK (E:FD, H:D6, L:FF)
...
avrdude -v -p atmega328pb -C +../lib/avrdude/328pb.conf -c stk500v1 -P /dev/ttyACM0 -b 19200 -e -U flash:w:manager.hex -U lock:w:0xEF:m
...
avrdude done.  Thank you.
```

## R-Pi Prep

```
sudo apt-get install i2c-tools python3-smbus
sudo usermod -a -G i2c your_username
# logout for the change to take
```


## RPUbus Addressing

The Address '1' on the multidrop serial bus is 0x31, (e.g., not 0x1 but the ASCII value for the character).

When HOST_nRTS is pulled active from a host trying to connect to the serial bus, the local bus manager will set localhost_active and send the bootloader_address over the DTR pair. If an address received by way of the DTR pair matches the local RPU_ADDRESS the bus manager will enter bootloader mode (marked with bootloader_started), and connect the shield RX/TX to the RS-422 (see connect_bootload_mode() function), all other addresses are locked out. After a LOCKOUT_DELAY time or when a normal mode byte is seen on the DTR pair, the lockout ends and normal mode resumes. The node that has bootloader_started broadcast the return to normal mode byte on the DTR pair when that node has the RPU_ADDRESS read from its bus manager over I2C (otherwise it will time out and not connect the controller RX/TX to serial).


## Bus Manager Modes

In Normal Mode, the manager connects the local application controller (ATmega324pb) to the RPUbus if it is RPU aware (e.g., ask for RPU_ADDRESS over I2C). Otherwise, it will not attach the local MCU. The host side of the transceivers connects unless it is foreign (e.g., keep out the locals when foreigners are around).

In bootload mode, the manager connects the local application controller to the multi-drop serial bus. Also, the host will be connected unless it is foreign. It is expected that all other nodes are in lockout mode. Note the BOOTLOADER_ACTIVE delay is less than the LOCKOUT_DELAY, but it needs to be in bootload mode long enough to allow uploading. A slow bootloader will require longer delays.

In lockout mode, if the host is foreign, both the local controller and Host are disconnected from the bus. Otherwise, the host remains connected.


## I2C and SMBus Interfaces

There are two TWI interfaces one acts as an I2C slave and is used to connect with the local microcontroller, while the other is an SMBus slave and connects with the local host (e.g., an R-Pi.) The commands sent are the same in both cases, but Linux does not like repeated starts or clock stretching so the SMBus read is done as a second bus transaction. I'm not sure the method is correct, but it seems to work, I echo back the previous transaction for an SMBus read. The masters sent (slave received) data is used to size the reply, so add a byte after the command for the manager to fill in with the reply. The I2C address is 0x29 (dec 41) and SMBus is 0x2A (dec 42). It is organized as an array of commands. 


[Point To Multi-Point] commands 0..15 (Ox00..0x0F | 0b00000000..0b00001111)

[Point To Multi-Point]: ./PointToMultiPoint.md

0. read the RPU_BUS address and activate normal mode (broadcast if localhost_active).
1. set the RPU_BUS address and write it to EEPROM.
2. read the address sent when DTR/RTS toggles.
3. write the address that will be sent when DTR/RTS toggles
4. read shutdown switch (the ICP1 pin has a weak pull-up and a momentary switch).
5. set shutdown switch (pull down ICP1 for SHUTDOWN_TIME to cause the host to halt).
6. reads status bits [0:DTR readback timeout, 1:twi transmit fail, 2:DTR readback not match, 3:host lockout].
7. writes (or clears) status.

[Point To Point] and Management commands (application controller must not read the manager address or it will switch back to multi-point) 16..31 (Ox10..0x1F | 0b00010000..0b00011111)

[Point To Point]: ./PointToPoint.md

16. set arduino_mode 
17. read arduino_mode


[Power Management] commands 32..47 (Ox20..0x2F | 0b00100000..0b00101111)

[Power Management]: ./PowerManagement.md

32. Analog channel 0 for ALT_I (uint16_t)
33. Analog channel 1 for ALT_V (uint16_t)
34. Analog channel 6 for PWR_I (uint16_t)
35. Analog channel 7 for PWR_V (uint16_t)
36. Analog timed accumulation for ALT_IT (uint32_t)
37. Analog timed accumulation for PWR_IT (uint32_t)
38. Analog referance for EXTERNAL_AVCC (uint32_t)
39. Analog referance for INTERNAL_1V1 (uint32_t)


[Test Mode] commands 48..63 (Ox30..0x3F | 0b00110000..0b00111111)

[Test Mode]: ./TestMode.md

48. save trancever control bits HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE for test_mode.
49. recover trancever control bits after test_mode.
50. read trancever control bits durring test_mode, e.g. 0b11101010 is HOST_nRTS = 1, HOST_nCTS =1, DTR_nRE =1, TX_nRE = 1, TX_DE =0, DTR_nRE =1, DTR_DE = 0, RX_nRE =1, RX_DE = 0.
51. set trancever control bits durring test_mode, e.g. 0b11101010 is HOST_nRTS = 1, HOST_nCTS =1, TX_nRE = 1, TX_DE =0, DTR_nRE =1, DTR_DE = 0, RX_nRE =1, RX_DE = 0.


Connect to i2c-debug on an RPUno with an RPU shield using picocom (or ilk). 

``` 
picocom -b 38400 /dev/ttyUSB0
``` 


## Scan I2C0 with i2c-debug

Scan for the address on I2C0.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/id?
{"id":{"name":"I2Cdebug^1","desc":"RPUno (14140^9) Board /w atmega328p","avr-gcc":"5.4.0"}}
/1/iscan?
{"scan":[{"addr":"0x29"}]}
```


## Scan I2C1 with Raspberry Pi

The I2C1 port is for SMBus access. A Raspberry Pi is set up as follows.

```
i2cdetect 1
WARNING! This program can confuse your I2C bus, cause data loss and worse!
I will probe file /dev/i2c-1.
I will probe address range 0x03-0x77.
Continue? [Y/n] Y
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- 2a -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- --
```

