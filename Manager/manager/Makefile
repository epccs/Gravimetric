# build manager from source for ATMega328pb on a Gravimetric board
# https://www.gnu.org/software/make/manual/make.html
TARGET = manager
LIBDIR = ../lib
# these have a restricted license, source is not published
OBJECTRISTRICT = main.o \
	calibration_limits.o
# these are LGPL license
OBJECTS = rpubus_manager_state.o \
	dtr_transmition.o \
	i2c_cmds.o \
	smbus_cmds.o \
	id_in_ee.o \
	adc_burst.o \
	references.o \
	power_manager.o \
	battery_limits.o \
	daynight_limits.o \
	daynight_state.o \
	$(LIBDIR)/uart0_bsd.o \
	$(LIBDIR)/adc_bsd.o \
	$(LIBDIR)/timers_bsd.o \
	$(LIBDIR)/twi0_bsd.o \
	$(LIBDIR)/twi1_bsd.o

# Chip and project-specific global definitions
MCU   =  atmega328pb
F_CPU = 12000000UL  

# defines for DTR_BAUD, I2C0_ADDRESS, I2C1_ADDRESS, RPU_ADDRESS, RPU_HOST_CONNECT now in main.h
# note BAUD is used as register on new AVR (m4809, AVR128DA)
CPPFLAGS = -DF_CPU=$(F_CPU) -I. 

#  http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega328p&LOW=FF&HIGH=D6&EXTENDED=05&LOCKBIT=3F
LOW_FUSE = 0xFF
HIGH_FUSE = 0xD6
EXTENDED_FUSE = 0xFD
UNLOCK_FUSE = 0xFF
LOCK_FUSE = 0xEF

# Cross-compilation
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size

# Uno with Arduino as ISP sketch shows as ttyACMx
ISP_PORT = /dev/ttyACM0

# Compiler/linker options
CFLAGS = -Os -g -std=gnu99 -Wall
# CFLAGS += -funsigned-char -funsigned-bitfields 
# CFLAGS += -fpack-struct -fshort-enums 
CFLAGS += -ffunction-sections -fdata-sections 

TARGET_ARCH = -mmcu=$(MCU) -B $(LIBDIR)/ATmega_DFP/gcc/dev/atmega328pb/ -I $(LIBDIR)/ATmega_DFP/include/
# if atmega328pb is in avr-gcc mainline use
#TARGET_ARCH = -mmcu=$(MCU)

LDFLAGS = -Wl,-Map,$(TARGET).map 
LDFLAGS += -Wl,--gc-sections 

.PHONY: help

# some help for the make impaired
# https://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
help:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

all: $(TARGET).hex $(TARGET).lst ## build the image and its related files

$(TARGET): $(TARGET).hex

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

# 328pb can lock the bootloader section which I may use at some point
isp: ## with Arduino as ISP sketch read the eeprom, set fuse, upload program image to flash
	avrdude -v -p $(MCU) -C +$(LIBDIR)/avrdude/328pb.conf -c stk500v1 -P $(ISP_PORT) -b 19200 -U eeprom:r:$(TARGET)_$(MCU)_eeprom.hex:i
	avrdude -v -p $(MCU) -C +$(LIBDIR)/avrdude/328pb.conf -c stk500v1 -P $(ISP_PORT) -b 19200 -e -U lock:w:$(UNLOCK_FUSE):m -U lfuse:w:$(LOW_FUSE):m -U hfuse:w:$(HIGH_FUSE):m -U efuse:w:$(EXTENDED_FUSE):m
	avrdude -v -p $(MCU) -C +$(LIBDIR)/avrdude/328pb.conf -c stk500v1 -P $(ISP_PORT) -b 19200 -e -U flash:w:$(TARGET).hex -U lock:w:$(LOCK_FUSE):m

# The avrdude in raspian is from
# https://github.com/facchinm/avrdude
linuxspi: ## with an R-Pi SPI port read the eeprom, set fuse, upload program image to flash
	sudo avrdude -v -p $(MCU) -C +$(LIBDIR)/avrdude/328pb.conf -c linuxspi -P /dev/spidev0.0 -U eeprom:r:$(TARGET)_$(MCU)_eeprom.hex:i
	sudo avrdude -v -p $(MCU) -C +$(LIBDIR)/avrdude/328pb.conf -c linuxspi -P /dev/spidev0.0 -e -U lock:w:$(UNLOCK_FUSE):m -U lfuse:w:$(LOW_FUSE):m -U hfuse:w:$(HIGH_FUSE):m -U efuse:w:$(EXTENDED_FUSE):m
	sudo avrdude -v -p $(MCU) -C +$(LIBDIR)/avrdude/328pb.conf -c linuxspi -P /dev/spidev0.0 -e -U flash:w:$(TARGET).hex -U lock:w:$(LOCK_FUSE):m

# recover a saved eeprom
recoveree: ## with Arduino as ISP sketch recover the saved eeprom
	avrdude -v -p $(MCU) -c stk500v1 -P $(ISP_PORT) -b 19200 -U eeprom:w:$(TARGET)_$(MCU)_eeprom.hex
	rm $(TARGET)_$(MCU)_eeprom.hex
	
ree_linuxspi: ## with an R-Pi SPI port recover the saved eeprom
	sudo avrdude -v -p $(MCU) -c linuxspi -P /dev/spidev0.0 -U eeprom:w:$(TARGET)_$(MCU)_eeprom.hex
	rm $(TARGET)_$(MCU)_eeprom.hex

# binutils (e.g. avr-size) is missing atmega328pb
# object file from restricted source license need to be kept and distrubuted so they can be linked with
# this allows others to modify the LGPL source and build objects that can link with the provided objects from the restricted source
$(TARGET).elf: $(OBJECTS) $(OBJECTRISTRICT)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ -o $@
	$(SIZE) -C --mcu=atmega328p $@
	rm -f $(TARGET).o $(OBJECTS)

clean: ## remove the image and related files
	rm -f $(TARGET).hex $(TARGET).map $(TARGET).elf $(TARGET).lst $(OBJECTRISTRICT)
 
%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@
