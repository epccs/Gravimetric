# build application from source
# when files for application change the binary image needs updated
# https://www.gnu.org/software/make/manual/make.html
TARGET = Adc
LIBDIR = ../lib
OBJECTS = main.o \
	analog.o \
	calibrate.o \
	references.o \
	../Uart/id.o \
	$(LIBDIR)/timers_bsd.o \
	$(LIBDIR)/uart0_bsd.o \
	$(LIBDIR)/twi0.o \
	$(LIBDIR)/rpu_mgr.o \
	$(LIBDIR)/adc_bsd.o \
	$(LIBDIR)/parse.o

## Chip and project-specific global definitions
MCU   =  atmega324pb
F_CPU = 16000000UL  
CPPFLAGS = -DF_CPU=$(F_CPU) -I. 

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

TARGET_ARCH = -mmcu=$(MCU) -B $(LIBDIR)/ATmega_DFP/gcc/dev/atmega324pb/ -I $(LIBDIR)/ATmega_DFP/include/
## if atmega324pb is in avr-gcc mainline use
##TARGET_ARCH = -mmcu=$(MCU)

LDFLAGS = -Wl,-Map,$(TARGET).map 
LDFLAGS += -Wl,--gc-sections 

## printf() is one of the vfprintf() family of functions which does not implement floating point conversion by default
## https://www.nongnu.org/avr-libc/user-manual/group__avr__stdio.html
LDFLAGS += -Wl,-u,vfprintf -lprintf_flt -lm

.PHONY: help

# some help for the make impaired
# https://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
help:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

all: $(TARGET).hex $(TARGET).lst ## build the image and its related files

$(TARGET): $(TARGET).hex

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

# optiboot erases flash without being told (e.g. -e )
bootload: ## upload to optiboot 
	avrdude -v -p $(MCU) -C +$(LIBDIR)/avrdude/324pb.conf -c arduino -P $(BOOTLOAD_PORT) -b 38400 -U flash:w:$(TARGET).hex

$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ -o $@
	@echo binutils-avr do not have 324pb, but it is sized like a 324p
	$(SIZE) -C --mcu=atmega324p $@
	rm -f $(TARGET).o $(OBJECTS)

clean: ## remove the image and its related files
	rm -f $(TARGET).hex $(TARGET).map $(TARGET).elf $(TARGET).lst
 
%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

