# Makefile 
TARGET = DayNight
LIBDIR = ../lib
OBJECTS = main.o \
	day_night.o \
	../Uart/id.o \
	$(LIBDIR)/timers.o \
	$(LIBDIR)/uart.o \
	$(LIBDIR)/twi0.o \
	$(LIBDIR)/rpu_mgr.o \
	$(LIBDIR)/adc.o \
	$(LIBDIR)/parse.o

## Chip and project-specific global definitions
MCU   =  atmega324pb
F_CPU = 16000000UL  
BAUD  =  38400UL
CPPFLAGS = -DF_CPU=$(F_CPU) -DBAUD=$(BAUD) -I. 

## Cross-compilation
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size

# FTDI's USB to serial bridge shows as /dev/ttyUSB0, 
# Uno's serial bridge (an ATmega16U2) shows as /dev/ttyACM0  (a modem,?)
# Pi Zero on chip hardware serial shows as /dev/ttyAMA0 (hardware UART on a Linux system)
detect_PORT := $(shell sh -c 'ls /dev/ttyAMA0 2>/dev/null || echo not')
ifeq ($(detect_PORT),/dev/ttyAMA0)
	BOOTLOAD_PORT = /dev/ttyAMA0
endif
detect_PORT := $(shell sh -c 'ls /dev/ttyUSB0 2>/dev/null || echo not')
ifeq ($(detect_PORT),/dev/ttyUSB0)
	BOOTLOAD_PORT = /dev/ttyUSB0
endif

## Compiler/linker options
CFLAGS = -Os -g -std=gnu99 -Wall
# CFLAGS += -funsigned-char -funsigned-bitfields 
# CFLAGS += -fpack-struct -fshort-enums 
CFLAGS += -ffunction-sections -fdata-sections 

TARGET_ARCH = -mmcu=$(MCU) -B $(LIBDIR)/Atmel.ATmega_DFP.1.3.300.atpack/gcc/dev/atmega324pb/ -I $(LIBDIR)/Atmel.ATmega_DFP.1.3.300.atpack/include/
## if atmega324pb is in avr-gcc mainline use
##TARGET_ARCH = -mmcu=$(MCU)

LDFLAGS = -Wl,-Map,$(TARGET).map 
LDFLAGS += -Wl,--gc-sections 

## printf() is one of the vfprintf() family of functions which does not implement floating point conversion by default
## warning this takes about 2k of program space
LDFLAGS += -Wl,-u,vfprintf -lprintf_flt -lm

all: $(TARGET).hex $(TARGET).lst

$(TARGET): $(TARGET).hex  

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

## works with optiboot which erases flash without being told (e.g. -e )
bootload:
	avrdude -v -p $(MCU) -C +$(LIBDIR)/avrdude/324pb.conf -c arduino -P $(BOOTLOAD_PORT) -b 38400 -U flash:w:$(TARGET).hex

$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ -o $@
	@echo binutils-avr do not have 324pb, but it is sized like a 324p
	$(SIZE) -C --mcu=atmega324p $@
	rm -f $(TARGET).o $(OBJECTS)

clean: 
	rm -f $(TARGET).hex $(TARGET).map $(TARGET).elf $(TARGET).lst
 
%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@