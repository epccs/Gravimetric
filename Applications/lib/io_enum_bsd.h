/* IO Enumeration
Copyright (C) 2020 Ronald Sutherland

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES 
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE 
FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY 
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, 
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, 
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

https://en.wikipedia.org/wiki/BSD_licenses#0-clause_license_(%22Zero_Clause_BSD%22)

Editors like VScode with IntelliSense can show appropriate options.
Some compilers can reject code that is an incorrect type.

Some referance material to help get the needed SBI/CBI instructions
https://www.avrfreaks.net/forum/c-gpio-class-inheritance-problems
https://github.com/greiman/SdFat-beta/blob/master/src/DigitalIO/DigitalPin.h
*/

#ifndef IO_Enum_h
#define IO_Enum_h

#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>
#include <util/atomic.h>

#if defined(__AVR_ATmega324PB__)

// Direction is used to program the IO as an input or output
typedef enum DIRECTION_enum {
    DIRECTION_INPUT,
    DIRECTION_OUTPUT
} DIRECTION_t;

// Logic Level
typedef enum LOGIC_LEVEL_enum {
    LOGIC_LEVEL_LOW,
    LOGIC_LEVEL_HIGH
} LOGIC_LEVEL_t;

// enumeraiton names for the MCU_IO_<node> on schematic
typedef enum MCU_IO_enum {
    MCU_IO_ADC0,  // PA0 can be used as GPIO or analog channel 0
    MCU_IO_ADC1, // PA1 can be used as GPIO or analog channel 1
    MCU_IO_ADC2, // PA2 can be used as GPIO or analog channel 2
    MCU_IO_ADC3, // PA3 can be used as GPIO or analog channel 3
    MCU_IO_ADC4, // PA4 connected to TP4 can be used as GPIO or analog channel 4
    MCU_IO_ADC5, // PA5 connected to TP5 can be used as GPIO or analog channel 5
    MCU_IO_ADC6, // PA6 connected to TP6 can be used as GPIO or analog channel 6
    MCU_IO_ADC7, // PA7 connected to TP7 can be used as GPIO or analog channel 7
    MCU_IO_CS0_EN, // PB0 pullup to enabled 22mA current source next to ADC0
    MCU_IO_CS4_EN, // PB1 pullup to enabled 22mA current source next to ADC4
    MCU_IO_NC10, // PB2 connected to TP10 can be used as GPIO
    MCU_IO_CS2_EN,  // PB3 pullup to enabled 22mA current source next to ADC2
    MCU_IO_nSS, // PB4 used for slave SPI as input from !CE00 R-Pi SPI master, durring ISP upload remove the R-Pi from its header.
                         // PB4 also used to mask the CS_ICP3 control of the 22mA current source for use with input captrue of Timer3. 
    MCU_IO_ICP3_MOSI, // PB5 used for slave SPI as input from BCM10/MOSI R-Pi SPI master, durring ISP upload remove the R-Pi from its header.
                                    // PB5 also used for input captrue of Timer3 (START event) durring which SPI slave input nSS must be HIGH.
    MCU_IO_MISO, // PB6 used for slave SPI as output to BCM9/MISO R-Pi SPI master, durring ISP upload remove the R-Pi from its header.
    MCU_IO_SCK,   // PB7 used for slave SPI as input from BCM11/SCK R-Pi SPI master, durring ISP upload remove the R-Pi from its header.
    MCU_IO_SCL0, // PC0 is for use as an I2C SCL master that connects to the manager.
    MCU_IO_SDA0, // PC1 is for use as an I2C SDA master that connects to the manager.
    MCU_IO_CS_FAST, // PC2 controls a 22mA current source for fast flow into gravimetric diversion control
                                // fast flow needs to be disabled durring the START and STOP events for best measurements
    MCU_IO_ICP4, // PC3 used for input captrue of Timer4, e.g., STOP event.
    MCU_IO_CS_ICP4, // PC4 controls a 22mA current source for use with ICP4 STOP events
    MCU_IO_CS_ICP3, // PC5 controls a 22mA current source for use with ICP3 START events. Control masked when nSS is LOW.
    MCU_IO_CS_DIVERSION, // PC6 controls a 22mA current source for gravimetric diversion control events. 
                                          // PC6 control is masked during ICP4 (STOP) one-shot (~1.1mSec), during which it needs to be turned off.
                                          // PC6 control overrides during ICP3 (START) one-shot (~1.1mSec), during which it needs to be turned on.
    MCU_IO_CS3_EN, // PC7 pullup to enabled 22mA current source next to ADC3
    MCU_IO_RX2, // PE2 is for use as GPIO or UART2 receiver.
    MCU_IO_TX2, // PE3 is for use as GPIO or UART2 transmitter. 
    // PE4 is for AREF
    MCU_IO_SDA1, // PE5 is an I2C SDA for use as a slave or master.
    MCU_IO_SCL1, // PE6 is an I2C SCL for use as a slave or master.
    MCU_IO_RX0, // PD0 is for use with multi-drop serial as UART0 receiver.
    MCU_IO_TX0, // PD1 is for usewith multi-drop serial as UART0 transmitter. 
    MCU_IO_RX1, // PD2 is for use as GPIO or UART1 receiver.
    MCU_IO_TX1, // PD3 is for use as GPIO or UART1 transmitter.
    MCU_IO_CS1_EN,  // PD4 pullup to enabled 22mA current source next to ADC1
    MCU_IO_CS_ICP1, // PD5 controls a 22mA current source for use with ICP1 FT events
    MCU_IO_ICP1, // PD6 used for input captrue of Timer1, e.g., FT events.
    MCU_IO_NC35, // PD7 connected to TP35 can be used as GPIO
    MCU_IO_END
} MCU_IO_t;

typedef volatile uint8_t register8_t;

struct IO_Map {
    // https://yarchive.net/comp/linux/typedefs.html
    register8_t* ddr; // data direction register
    register8_t* in;  // input register
    register8_t* port; // output register
    uint8_t mask;  
};


// map of direct port manipulation registers used for IO functions.
const static struct IO_Map ioMap[MCU_IO_END] = 
{
    [MCU_IO_ADC0] = { .ddr=&DDRA, .in=&PINA, .port=&PORTA, .mask= 1<<PA0 },
    [MCU_IO_ADC1] = { .ddr=&DDRA, .in=&PINA, .port=&PORTA, .mask= 1<<PA1 },
    [MCU_IO_ADC2] = { .ddr=&DDRA, .in=&PINA, .port=&PORTA, .mask= 1<<PA2 },
    [MCU_IO_ADC3] = { .ddr=&DDRA, .in=&PINA, .port=&PORTA, .mask= 1<<PA3 },
    [MCU_IO_ADC4] = { .ddr=&DDRA, .in=&PINA, .port=&PORTA, .mask= 1<<PA4 },
    [MCU_IO_ADC5] = { .ddr=&DDRA, .in=&PINA, .port=&PORTA, .mask= 1<<PA5 },
    [MCU_IO_ADC6] = { .ddr=&DDRA, .in=&PINA, .port=&PORTA, .mask= 1<<PA6 },
    [MCU_IO_ADC7] = { .ddr=&DDRA, .in=&PINA, .port=&PORTA, .mask= 1<<PA7 },
    [MCU_IO_CS0_EN] = { .ddr=&DDRB, .in=&PINB, .port=&PORTB, .mask= 1<<PB0 },
    [MCU_IO_CS4_EN] = { .ddr=&DDRB, .in=&PINB, .port=&PORTB, .mask= 1<<PB1 },
    [MCU_IO_NC10] = { .ddr=&DDRB, .in=&PINB, .port=&PORTB, .mask= 1<<PB2 },
    [MCU_IO_CS2_EN] = { .ddr=&DDRB, .in=&PINB, .port=&PORTB, .mask= 1<<PB3 },
    [MCU_IO_nSS] = { .ddr=&DDRB, .in=&PINB, .port=&PORTB, .mask= 1<<PB4 },
    [MCU_IO_ICP3_MOSI] = { .ddr=&DDRB, .in=&PINB, .port=&PORTB, .mask= 1<<PB5 },
    [MCU_IO_MISO] = { .ddr=&DDRB, .in=&PINB, .port=&PORTB, .mask= 1<<PB6 },
    [MCU_IO_SCK] = { .ddr=&DDRB, .in=&PINB, .port=&PORTB, .mask= 1<<PB7 },
    [MCU_IO_SCL0] = { .ddr=&DDRC, .in=&PINC, .port=&PORTC, .mask= 1<<PC0 },
    [MCU_IO_SDA0] = { .ddr=&DDRC, .in=&PINC, .port=&PORTC, .mask= 1<<PC1 },
    [MCU_IO_CS_FAST] = { .ddr=&DDRC, .in=&PINC, .port=&PORTC, .mask= 1<<PC2 },
    [MCU_IO_ICP4] = { .ddr=&DDRC, .in=&PINC, .port=&PORTC, .mask= 1<<PC3 },
    [MCU_IO_CS_ICP4] = { .ddr=&DDRC, .in=&PINC, .port=&PORTC, .mask= 1<<PC4 },
    [MCU_IO_CS_ICP3] = { .ddr=&DDRC, .in=&PINC, .port=&PORTC, .mask= 1<<PC5 },
    [MCU_IO_CS_DIVERSION] = { .ddr=&DDRC, .in=&PINC, .port=&PORTC, .mask= 1<<PC6 },
    [MCU_IO_CS3_EN] = { .ddr=&DDRC, .in=&PINC, .port=&PORTC, .mask= 1<<PC7 },
    [MCU_IO_RX2] = { .ddr=&DDRE, .in=&PINE, .port=&PORTE, .mask= 1<<PE2 },
    [MCU_IO_TX2] = { .ddr=&DDRE, .in=&PINE, .port=&PORTE, .mask= 1<<PE3 },
    [MCU_IO_SDA1] = { .ddr=&DDRE, .in=&PINE, .port=&PORTE, .mask= 1<<PE5 },
    [MCU_IO_SCL1] = { .ddr=&DDRE, .in=&PINE, .port=&PORTE, .mask= 1<<PE6 },
    [MCU_IO_RX0] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD0 },
    [MCU_IO_TX0] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD1 }, 
    [MCU_IO_RX1] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD2 },
    [MCU_IO_TX1] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD3 },
    [MCU_IO_CS1_EN] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD4 },
    [MCU_IO_CS_ICP1] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD5 },
    [MCU_IO_ICP1] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD6 },
    [MCU_IO_NC35] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD7 }
};
#else
#   error this is for an mega324pb application controller on https://github.com/epccs/Gravimetric
#endif

// read value from IO input bit and return its bool value
static inline __attribute__((always_inline))
bool ioRead(MCU_IO_t io) 
{
    return (*ioMap[io].in & ioMap[io].mask);
}

// set or clear IO output
static inline __attribute__((always_inline))
void ioWrite(MCU_IO_t io, LOGIC_LEVEL_t level) {
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE )
    {
        if (level) // == LOGIC_LEVEL_HIGH 
        {
            *ioMap[io].port |= ioMap[io].mask;
        } 
        else 
        {
            *ioMap[io].port &= ~ioMap[io].mask;
        }
    }
}

// toggle io
static inline __attribute__((always_inline))
void ioToggle(MCU_IO_t io) {
    ioWrite(io, (LOGIC_LEVEL_t) !ioRead(io));
    /* keep it simple
    if (ioMap[io].in > ((register8_t *)(0X3F)) )  //I/O space is 0..0x3F
    {
        // https://www.microchip.com/webdoc/AVRLibcReferenceManual/group__avr__sfr__notes.html
        
    }
    else
    {
        // somehow this does sbi/cbi on the bit in PORTx register, but it may quit doing that at any time.
        *ioMap[io].in |= ioMap[io].mask;
    } 
    */
}

// set io direction (INPUT or OUTPUT).
static inline __attribute__((always_inline))
void ioDir(MCU_IO_t io, DIRECTION_t dir) 
{
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE )
    {
        if (dir) // == DIRECTION_OUTPUT 
        {
            *ioMap[io].ddr |= ioMap[io].mask;
        } 
        else 
        {
            *ioMap[io].ddr &= ~ioMap[io].mask;
        }
    }
}

#endif // IO_Enum_h
