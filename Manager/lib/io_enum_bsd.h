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

#if defined(__AVR_ATmega328PB__)

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

// enumeraiton names for the MCU_IO_<node> is on schematic
typedef enum MCU_IO_enum {
    MCU_IO_ALT_I,  // PC0 has analog channel 0 to measure  alternate current with 0.018 Ohm sense and a gain of 50 amplifier
    MCU_IO_ALT_V, // PC1 has analog channel 1 to measued alternate voltage with 100k and 10k Ohm divider
    MCU_IO_TX_nRE, // PC2 controls TX pair transceiver receiver disable.
    MCU_IO_RX_DE, // PC3 controls RX pair transceiver driver enable.
    MCU_IO_SDA0, // PC4 is an I2C SDA slave and connects the manager with application uC.
    MCU_IO_SCL0, // PC5 is an I2C SCL slave and connects the manager with application uC.
    MCU_IO_PWR_I, // PE2 has analog channel 6 to measure power current with 0.068 Ohm sense and a gain of 50 amplifier
    MCU_IO_PWR_V, // PE3 has analog channel 7 to measue power voltage with 100k and 15.8k Ohm divider
    MCU_IO_SHUTDOWN, // PB0 needs a weak pullup enabled so a manual switch can HALT the R-Pi on BCM6 (pin 31 on SBC header) 
    MCU_IO_PIPWR_EN, // PB1 has a 10k Ohm pull-up to enable power to the R-Pi (SBC), pull down with push-pull (OUTPUT mode) to turn off power.
    MCU_IO_MGR_nSS, // PB2 can pull down to reset the application uC (e.g., to run the bootloader).
    MCU_IO_ALT_EN,  // PB3 can pull up to enable the alternat power input, it has a 10k pull down on the board. ISP: remove alternat power befor programing with ISP.
    MCU_IO_MGR_MISO, // PB4 use for ISP only
    MCU_IO_MGR_SCK_LED,   // PB5 has red LED for manager status and is used for ISP
    MCU_IO_DTR_RXD,   // PD0 has UART0 Tx used with half-duplex DTR pair for multi-drop bus state.
    MCU_IO_DTR_TXD,   // PD1 has UART0 Rx used with half duplex DTR pair
    MCU_IO_HOST_nCTS, // PD2 tells the host not to use the serial, e.g., controls host nCTS handshake.
    MCU_IO_HOST_nRTS, // PD3 lets the host start a point to point connection to a target running a bootloader (or has a UART based programming interface), e.g., it reads the host nRTS handshake.
    MCU_IO_RX_nRE, // PD4 controls RX pair transceiver receiver disable.
    MCU_IO_TX_DE, // PD5 controls TX pair transceiver driver enable.
    MCU_IO_DTR_nRE, // PD6 controls DTR pair transceiver receiver disable.
    MCU_IO_DTR_DE, // PD7 controls DTR pair transceiver driver enable.
    MCU_IO_SDA1, // PE0 is an SMBus SDA slave and connects the manager to the R-Pi (SBC).
    MCU_IO_SCL1, // PE1 is an SMBus SCL slave and connects the manager to the R-Pi (SBC).
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
    [MCU_IO_ALT_I] = { .ddr=&DDRC, .in=&PINC, .port=&PORTC, .mask= 1<<PC0 },
    [MCU_IO_ALT_V] = { .ddr=&DDRC, .in=&PINC, .port=&PORTC, .mask= 1<<PC1 },
    [MCU_IO_TX_nRE] = { .ddr=&DDRC, .in=&PINC, .port=&PORTC, .mask= 1<<PC2 },
    [MCU_IO_RX_DE] = { .ddr=&DDRC, .in=&PINC, .port=&PORTC, .mask= 1<<PC3 },
    [MCU_IO_SDA0] = { .ddr=&DDRC, .in=&PINC, .port=&PORTC, .mask= 1<<PC4 },
    [MCU_IO_SCL0] = { .ddr=&DDRC, .in=&PINC, .port=&PORTC, .mask= 1<<PC5 },
    [MCU_IO_PWR_I] = { .ddr=&DDRE, .in=&PINE, .port=&PORTE, .mask= 1<<PE2 },
    [MCU_IO_PWR_V] = { .ddr=&DDRE, .in=&PINE, .port=&PORTE, .mask= 1<<PE3 },
    [MCU_IO_SHUTDOWN] = { .ddr=&DDRB, .in=&PINB, .port=&PORTB, .mask= 1<<PB0 }, 
    [MCU_IO_PIPWR_EN] = { .ddr=&DDRB, .in=&PINB, .port=&PORTB, .mask= 1<<PB1 },
    [MCU_IO_MGR_nSS] = { .ddr=&DDRB, .in=&PINB, .port=&PORTB, .mask= 1<<PB2 },
    [MCU_IO_ALT_EN] = { .ddr=&DDRB, .in=&PINB, .port=&PORTB, .mask= 1<<PB3 },
    [MCU_IO_MGR_MISO] = { .ddr=&DDRB, .in=&PINB, .port=&PORTB, .mask= 1<<PB4 },
    [MCU_IO_MGR_SCK_LED] = { .ddr=&DDRB, .in=&PINB, .port=&PORTB, .mask= 1<<PB5 },
    [MCU_IO_DTR_RXD] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD0 },
    [MCU_IO_DTR_TXD] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD1 },
    [MCU_IO_HOST_nCTS] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD2 },
    [MCU_IO_HOST_nRTS] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD3 },
    [MCU_IO_RX_nRE] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD4 },
    [MCU_IO_TX_DE] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD5 },
    [MCU_IO_DTR_nRE] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD6 },
    [MCU_IO_DTR_DE] = { .ddr=&DDRD, .in=&PIND, .port=&PORTD, .mask= 1<<PD7 },
    [MCU_IO_SDA1] = { .ddr=&DDRE, .in=&PINE, .port=&PORTE, .mask= 1<<PE0 },
    [MCU_IO_SCL1] = { .ddr=&DDRE, .in=&PINE, .port=&PORTE, .mask= 1<<PE1 }
};
#else
#   error this is for an mega328pb manager on a PCB board, see https://github.com/epccs/Gravimetric
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
