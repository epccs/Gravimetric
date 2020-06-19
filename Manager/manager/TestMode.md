# Test Mode Commands

48..63 (Ox30..0x3F | 0b00110000..0b00111111)

48. save trancever control bits HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE for test_mode.
49. recover trancever control bits after test_mode.
50. read trancever control bits durring test_mode, e.g. 0b11101010 is HOST_nRTS = 1, HOST_nCTS =1, DTR_nRE =1, TX_nRE = 1, TX_DE =0, DTR_nRE =1, DTR_DE = 0, RX_nRE =1, RX_DE = 0.
51. set trancever control bits durring test_mode, e.g. 0b11101010 is HOST_nRTS = 1, HOST_nCTS =1, TX_nRE = 1, TX_DE =0, DTR_nRE =1, DTR_DE = 0, RX_nRE =1, RX_DE = 0.
52. not used.
53. not used.
54. not used.
55. not used.

Note: evening_debouce and morning_debouce are used for the day-night state machine, the command number may change at some point. 

## Cmd 48 from a controller /w i2c-debug set transceiver test mode

Set test_mode, use the bootload port interface since the RPUbus transceivers are turned off.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 48,1
{"txBuffer[2]":[{"data":"0x30"},{"data":"0x1"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x30"},{"data":"0x1"}]}
``` 

on the second I2C channel that is for SMBus's i2c block commands

``` 
picocom -b 38400 /dev/ttyUSB0
/0/iaddr 42
{"address":"0x2A"}
/0/ibuff 48,1
{"txBuffer[2]":[{"data":"0x30"},{"data":"0x1"}]}
/0/iwrite
{"returnCode":"success"}
/0/ibuff 48
{"txBuffer[1]":[{"data":"0x30"}]}
/0/iread? 2
{"rxBuffer":[{"data":"0x30"},{"data":"0x1"}]}
```

The first read does a write. SMBus then does a second write with the command and then a repeated start followed by reading the data. I am using a old buffer to supply the data from the first write. The command byte needs to match to setup the old buffer data for use by the transmit event callback. 


## Cmd 48 from a Raspberry Pi set transceiver test mode

Set test_mode with an R-Pi over SMBus.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# what is my address
bus.write_i2c_block_data(42, 0, [255])
bus.read_i2c_block_data(42, 0, 2)
[0, 50]
# set the bootload address to my address
bus.write_i2c_block_data(42, 3, [50])
bus.read_i2c_block_data(42, 3, 2)
[3, 50]
# clear the host lockout status bit so serial from this host can work
bus.write_i2c_block_data(42, 7, [0])
print(bus.read_i2c_block_data(42,7, 2))
[7, 0]
exit()
# on an RPUno load the blinkLED application which uses the UART
git clone https://github.com/epccs/RPUno
cd /RPUno/BlinkLED
make
make bootload
# now back to 
python3
import smbus
bus = smbus.SMBus(1)
# and set the test_mode
bus.write_i2c_block_data(42, 48, [1])
print(bus.read_i2c_block_data(42, 48, 2))
[48, 1]
``` 

I had picocom in another SSH connection to see how the test mode was working (e.g., it turns off the transceivers and serial stops operating). 

``` 
picocom -b 38400 /dev/ttyAMA0
a

``` 

Use  command 50 to set the transceiver control bits and check them with picocom.


## Cmd 49 from a controller /w i2c-debug recover after transceiver test

Recover trancever control bits after test_mode.

data returned is the recoverd trancever control bits: HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 49,1
{"txBuffer[2]":[{"data":"0x31"},{"data":"0x1"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x31"},{"data":"0x15"}]}
``` 

on the second I2C channel that is for SMBus's i2c block commands

``` 
picocom -b 38400 /dev/ttyUSB0
/0/iaddr 42
{"address":"0x2A"}
/0/ibuff 49,1
{"txBuffer[2]":[{"data":"0x31"},{"data":"0x1"}]}
/0/iwrite
{"returnCode":"success"}
/0/ibuff 49
{"txBuffer[1]":[{"data":"0x31"}]}
/0/iread? 2
{"rxBuffer":[{"data":"0x31"},{"data":"0x15"}]}
``` 

The read has the old buffer from the write command, it show data that can be seen after power up e.g., 0x15 or 0b00010101. The trancever control bits are: HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE


## Cmd 49 from a Raspberry Pi recover after transceiver test

Set test_mode with an R-Pi over SMBus.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# end the test_mode data byte is replaced with the recoverd trancever control
bus.write_i2c_block_data(42, 49, [1])
print(bus.read_i2c_block_data(42, 49, 2))
[49, 21]
bin(21)
'0b10101'
``` 

The trancever control bits are: HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE

So everything was enabled, it is sort of a stealth mode after power-up, and can allow a host to talk to new controllers on the bus if they don't toggle there RTS lines (e.g., the test case is an R-Pi on an RPUpi shield on an RPUno, all freshly powered.) Stealth mode ends when a byte is seen on the DTR pair, that is what will establish an accurate bus state, so stealth mode is an artifact of laziness after power up and may need to change. 


## Cmd 50 from a controller /w i2c-debug read trancever control bits

Read trancever control bits (HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE) durring test_mode on bootload port with i2c-debug.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 50,255
{"txBuffer[2]":[{"data":"0x32"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x32"},{"data":"0x22"}]}
``` 

on the second I2C channel that is for SMBus's i2c block commands

``` 
picocom -b 38400 /dev/ttyUSB0
/0/iaddr 42
{"address":"0x2A"}
/0/ibuff 50,255
{"txBuffer[2]":[{"data":"0x32"},{"data":"0xFF"}]}
/0/iwrite
{"returnCode":"success"}
/0/ibuff 50
{"txBuffer[1]":[{"data":"0x32"}]}
/0/iread? 2
{"rxBuffer":[{"data":"0x32"},{"data":"0x22"}]}
```


## Cmd 50 from a Raspberry Pi read trancever control bits

Read trancever control bits (HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE) durring test_mode with an R-Pi over SMBus.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# end the test_mode data byte is replaced with the recoverd trancever control
bus.write_i2c_block_data(42, 50, [255])
print(bus.read_i2c_block_data(42, 50, 2))
[50, 34]
``` 


## Cmd 51 from a controller /w i2c-debug set trancever control bits

Set trancever control bits (HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE) durring test_mode on bootload port with i2c-debug.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 51,38
{"txBuffer[2]":[{"data":"0x33"},{"data":"0x26"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x33"},{"data":"0x26"}]}
``` 

0x26 = HOST_nRTS==0:HOST_nCTS=0:TX_nRE==1:TX_DE==0:DTR_nRE==0:DTR_DE==1:RX_nRE==1:RX_DE==0 e.g., DTR trancever is on.

on the second I2C channel that is for SMBus's i2c block commands

``` 
picocom -b 38400 /dev/ttyUSB0
/0/iaddr 42
{"address":"0x2A"}
/0/ibuff 51,38
{"txBuffer[2]":[{"data":"0x33"},{"data":"0x26"}]}
/0/iwrite
{"returnCode":"success"}
/0/ibuff 51
{"txBuffer[1]":[{"data":"0x33"}]}
/0/iread? 2
{"rxBuffer":[{"data":"0x33"},{"data":"0x26"}]}
```


## Cmd 51 from a Raspberry Pi read trancever control bits

Set trancever control bits (HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE) durring test_mode with an R-Pi over SMBus.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
#read_i2c_block_data(I2C_ADDR, I2C_COMMAND, NUM_OF_BYTES)
# end the test_mode data byte is replaced with the recoverd trancever control
bus.write_i2c_block_data(42, 51, [38])
print(bus.read_i2c_block_data(42, 51, 2))
[51, 38]
``` 


## Cmd 52 is not used.

repurpose

## Cmd 53 is not used.

repurpose


## Cmd 54 is not used.

repurpose



## Cmd TBD from a controller /w i2c-debug to read manager millis time

Read the manager timer (e.g., millis).

The microcontroller uses Timer0 to count crystal oscillations, it should be accurate to 30ppm, but it uses the timer interrupt to update the unsigned long used to hold millis value. The update rate average is 976.5625 (16000000/(256*64)) updates per second. Therefore the counts will increment twice about 1 in 49 updates. After 2**32 mSec or 49.7 days (e.g., (2**32)/1000/60/60/24 ), a counter role over will occur.

``` 
/1/ibuff 54,0,0,0,0
{"txBuffer[5]":[{"data":"0x36"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 5
{"rxBuffer":[{"data":"0x36"},{"data":"0x0"},{"data":"0xF8"},{"data":"0xF9"},{"data":"0x13"}]}
/1/ibuff 54,0,0,0,0
{"txBuffer[5]":[{"data":"0x36"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"},{"data":"0x0"}]}
/1/iread? 5
{"rxBuffer":[{"data":"0x36"},{"data":"0x0"},{"data":"0xF9"},{"data":"0x83"},{"data":"0xA6"}]}
```

To understand how millis is used have a look at this link.

https://learn.adafruit.com/multi-tasking-the-arduino-part-1/using-millis-for-timing

On a host machine use its time keeping (e.g., man gettimeofday) not millis.

```
GETTIMEOFDAY(2)                        Linux Programmer's Manual                       GETTIMEOFDAY(2)

NAME
       gettimeofday, settimeofday - get / set time

SYNOPSIS
       #include <sys/time.h>

       int gettimeofday(struct timeval *tv, struct timezone *tz);

       int settimeofday(const struct timeval *tv, const struct timezone *tz);

   Feature Test Macro Requirements for glibc (see feature_test_macros(7)):

       settimeofday():
           Since glibc 2.19:
               _DEFAULT_SOURCE
           Glibc 2.19 and earlier:
               _BSD_SOURCE

DESCRIPTION
       The functions gettimeofday() and settimeofday() can get and set the time as well as a timezone.
       The tv argument is a struct timeval (as specified in <sys/time.h>):

           struct timeval {
               time_t      tv_sec;     /* seconds */
               suseconds_t tv_usec;    /* microseconds */
           };

       and gives the number of seconds and microseconds since the Epoch (see time(2)).  The  tz  argu-
       ment is a struct timezone:
           struct timezone {
               int tz_minuteswest;     /* minutes west of Greenwich */
               int tz_dsttime;         /* type of DST correction */
           };

       If either tv or tz is NULL, the corresponding structure is not set or returned.  (However, com-
       pilation warnings will result if tv is NULL.)

       The use of the timezone structure is obsolete; the tz argument should normally be specified  as
       NULL.  (See NOTES below.)

       Under  Linux, there are some peculiar "warp clock" semantics associated with the settimeofday()
       system call if on the very first call (after booting) that has a non-NULL tz argument,  the  tv
       argument is NULL and the tz_minuteswest field is nonzero.  (The tz_dsttime field should be zero
       for this case.)  In such a case it is assumed that the CMOS clock is on local time, and that it
       has  to be incremented by this amount to get UTC system time.  No doubt it is a bad idea to use
       this feature.
```

And some parts copied from mpbench for referance.

``` C
#define TIMER_CLEAR     (tv1.tv_sec = tv1.tv_usec = tv2.tv_sec = tv2.tv_usec = 0)
#define TIMER_START     gettimeofday(&tv1, (struct timezone*)0)
#define TIMER_ELAPSED   ((tv2.tv_usec-tv1.tv_usec)+((tv2.tv_sec-tv1.tv_sec)*1000000))
#define TIMER_STOP      gettimeofday(&tv2, (struct timezone*)0)
struct timeval tv1,tv2;

....
main()
...
    TIMER_CLEAR;
    TIMER_START;

    /* stuff to time goes here */

    TIMER_STOP;
    printf("# threads, calls, seconds, calls/s\n");
    printf("%d,%d,%f,%f\n", omp_get_num_threads(), counter, TIMER_ELAPSED/1000000.0, counter / (TIMER_ELAPSED/1000000.0));
...
```
