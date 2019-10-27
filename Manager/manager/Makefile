# manager for ATMega328pb on a RPUBUS board
#
TARGET = manager
LIBDIR = ../lib
OBJECTS = main.o \
	rpubus_manager_state.o \
	dtr_transmition.o \
	i2c_cmds.o \
	smbus_cmds.o \
	id_in_ee.o \
	adc_burst.o \
	references.o \
	power_manager.o \
	$(LIBDIR)/uart.o \
	$(LIBDIR)/adc.o \
	$(LIBDIR)/timers.o \
	$(LIBDIR)/twi0.o \
	$(LIBDIR)/twi1.o

## Chip and project-specific global definitions
MCU   =  atmega328pb
F_CPU = 12000000UL  
BAUD  =  250000UL
I2C0_ADDRESS = 0x29
I2C1_ADDRESS = 0x2A
RPU_ADDRESS = 0x31
# '0' or ascii 0x30 is default address sent on DTR pair when host connects,  e.g. pulls nDTR or nRTS active
RPU_HOST_CONNECT = 0x30
CPPFLAGS = -DRPU_HOST_CONNECT=$(RPU_HOST_CONNECT) -DI2C0_ADDRESS=$(I2C0_ADDRESS) -DI2C1_ADDRESS=$(I2C1_ADDRESS) -DRPU_ADDRESS=$(RPU_ADDRESS) -DF_CPU=$(F_CPU) -DBAUD=$(BAUD) -I. 

#  http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega328p&LOW=FF&HIGH=D6&EXTENDED=05&LOCKBIT=3F
LOW_FUSE = 0xFF
HIGH_FUSE = 0xD6
EXTENDED_FUSE = 0xFD
UNLOCK_FUSE = 0xFF
LOCK_FUSE = 0xEF

# at power up send a byte on the DTR pair to unlock the bus
# CPPFLAGS += -DDISCONNECT_AT_PWRUP=1

# at power up set HOST_LOCKOUT_STATUS bit
CPPFLAGS += -DHOST_LOCKOUT=1

## Cross-compilation
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size

## Uno with Arduino as ISP sketch shows as ttyACMx
ISP_PORT = /dev/ttyACM0

## Compiler/linker options
CFLAGS = -Os -g -std=gnu99 -Wall
# CFLAGS += -funsigned-char -funsigned-bitfields 
# CFLAGS += -fpack-struct -fshort-enums 
CFLAGS += -ffunction-sections -fdata-sections 

TARGET_ARCH = -mmcu=$(MCU) -B $(LIBDIR)/Atmel.ATmega_DFP.1.3.300.atpack/gcc/dev/atmega328pb/ -I $(LIBDIR)/Atmel.ATmega_DFP.1.3.300.atpack/include/
## if atmega328pb is in avr-gcc mainline use
##TARGET_ARCH = -mmcu=$(MCU)

LDFLAGS = -Wl,-Map,$(TARGET).map 
LDFLAGS += -Wl,--gc-sections 

all: $(TARGET).hex $(TARGET).lst

$(TARGET): $(TARGET).hex

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

## 328pb can lock the bootloader section which I may use at some point
isp:
	avrdude -v -p $(MCU) -C +$(LIBDIR)/avrdude/328pb.conf -c stk500v1 -P $(ISP_PORT) -b 19200 -U eeprom:r:$(TARGET)_$(MCU)_eeprom.hex:i
	avrdude -v -p $(MCU) -C +$(LIBDIR)/avrdude/328pb.conf -c stk500v1 -P $(ISP_PORT) -b 19200 -e -U lock:w:$(UNLOCK_FUSE):m -U lfuse:w:$(LOW_FUSE):m -U hfuse:w:$(HIGH_FUSE):m -U efuse:w:$(EXTENDED_FUSE):m
	avrdude -v -p $(MCU) -C +$(LIBDIR)/avrdude/328pb.conf -c stk500v1 -P $(ISP_PORT) -b 19200 -e -U flash:w:$(TARGET).hex -U lock:w:$(LOCK_FUSE):m

# The avrdude in raspian is from
# https://github.com/facchinm/avrdude
linuxspi:
	sudo avrdude -v -p $(MCU) -C +$(LIBDIR)/avrdude/328pb.conf -c linuxspi -P /dev/spidev0.0 -U eeprom:r:$(TARGET)_$(MCU)_eeprom.hex:i
	sudo avrdude -v -p $(MCU) -C +$(LIBDIR)/avrdude/328pb.conf -c linuxspi -P /dev/spidev0.0 -e -U lock:w:$(UNLOCK_FUSE):m -U lfuse:w:$(LOW_FUSE):m -U hfuse:w:$(HIGH_FUSE):m -U efuse:w:$(EXTENDED_FUSE):m
	sudo avrdude -v -p $(MCU) -C +$(LIBDIR)/avrdude/328pb.conf -c linuxspi -P /dev/spidev0.0 -e -U flash:w:$(TARGET).hex -U lock:w:$(LOCK_FUSE):m

# recover a saved eeprom
recoveree:
	avrdude -v -p $(MCU) -c stk500v1 -P $(ISP_PORT) -b 19200 -U eeprom:w:$(TARGET)_$(MCU)_eeprom.hex
	rm $(TARGET)_$(MCU)_eeprom.hex
	
ree_linuxspi:
	sudo avrdude -v -p $(MCU) -c linuxspi -P /dev/spidev0.0 -U eeprom:w:$(TARGET)_$(MCU)_eeprom.hex
	rm $(TARGET)_$(MCU)_eeprom.hex

# binutils (e.g. avr-size) is not getting updates (e.g., atmega328pb is missing)
# I think only Microchip can fix this issue
$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ -o $@
	$(SIZE) -C --mcu=atmega328p $@
	rm -f $(TARGET).o $(OBJECTS)

clean: 
	rm -f $(TARGET).hex $(TARGET).map $(TARGET).elf $(TARGET).lst
 
%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@
