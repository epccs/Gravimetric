/* DigitalIO Library
  Copyright (C) 2019 Ronald Sutherland
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
   Hacked from William Greiman to work in C with my board
   Functions are inspired by Wiring from Hernando Barragan
 */
#ifndef PinNum_h
#define PinNum_h

// avr-libc
#include <avr/io.h>
#include <util/atomic.h>

// avr-gcc
#include <stdbool.h>

#define INPUT 0
#define OUTPUT 1

#define LOW 0
#define HIGH 1

typedef struct {
  volatile uint8_t* ddr; 
  volatile uint8_t* pin; 
  volatile uint8_t* port;
  uint8_t bit;  
} Pin_Map;

#if defined(__AVR_ATmega324PB__)

#define NUM_DIGITAL_PINS 36

/* Each of the AVR Digital I/O ports is associated with three I/O registers. 
8 bit Data Direction Register (DDRx) each bit sets a pin as input (=0) or output (=1).
8 bit Port Input Register (PINx) each  bit is the input from a pin that was latched durring last low edge of the system clock.
8 bit Port Data Register (PORTx) each bit drives a pin if set as output (or sets pullup if input)
Where x is the port A, B, C, etc.

Wiring uses pin numbers to control their functions. */
static const Pin_Map pinMap[NUM_DIGITAL_PINS] = {
    [0] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PA0 }, // ADC0
    [1] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PA1 }, // ADC1
    [2] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PA2 }, // ADC2
    [3] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PA3 }, // ADC3
    [4] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PA4 }, // ADC4
    [5] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PA5 }, //  ADC5
    [6] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PA6 }, //  ADC6
    [7] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PA7 }, //  ADC7
    [8] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB0 }, // CS0_EN
    [9] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB1 }, // CS4_EN
    [10] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB2 }, // SHLD_VIN_EN
    [11] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB3 }, // CS2_EN
    [12] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB4 }, // nSS
    [13] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB5 }, // ICP3/MOSI
    [14] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB6 }, // MISO
    [15] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB7 }, // SCK
    [16] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC0 }, // SCL0
    [17] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC1 }, // SDA0
    [18] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC2 }, // CS_FAST
    [19] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC3 }, //  ICP4
    [20] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC4 }, // CS_ICP4
    [21] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC5 }, //  CS_ICP3
    [22] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC6 }, //  CS_DIVERSION
    [23] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC7 }, //  CS3_EN
    [24] = { .ddr=&DDRE, .pin=&PINE, .port=&PORTE, .bit= PE2 }, // RX2
    [25] = { .ddr=&DDRE, .pin=&PINE, .port=&PORTE, .bit= PE3 }, // TX2
    [26] = { .ddr=&DDRE, .pin=&PINE, .port=&PORTE, .bit= PE4 }, // SDA1
    [27] = { .ddr=&DDRE, .pin=&PINE, .port=&PORTE, .bit= PE5 }, // SCL1
    [28] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD0 }, // RX0
    [29] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD1 }, // TX0
    [30] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD2 }, // RX1
    [31] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD3 }, // TX1
    [32] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD4 }, // CS1_EN
    [33] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD5 }, // CS_ICP1
    [34] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD6 }, // ICP1
    [35] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD7 } // ALT_EN
};
#endif

// This is used to ignore bad pin numbers.
static inline __attribute__((always_inline)) uint8_t badPin(uint8_t pin) 
{
    if (pin >= NUM_DIGITAL_PINS) 
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static inline __attribute__((always_inline))
void bitWrite(volatile uint8_t* register_addr, uint8_t bit_offset, bool value_for_bit) 
{
    // Although I/O Registers 0x20 and 0x5F, e.g. PORTn and DDRn should not need 
    // atomic change control it does not harm.
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE )
    {
        if (value_for_bit) 
        {
            *register_addr |= 1 << bit_offset;
        } 
        else 
        {
            *register_addr &= ~(1 << bit_offset);
        }
    }
}

/* read value from pin number */
static inline __attribute__((always_inline))
bool digitalRead(uint8_t pin_num) 
{
    if (!badPin(pin_num)) 
    {
        return (*pinMap[pin_num].pin >> pinMap[pin_num].bit) & 1;
    }
    else
    {
        return 0;
    }
}

/* set pin value HIGH and LOW */
static inline __attribute__((always_inline))
void digitalWrite(uint8_t pin_num, bool value_for_bit) {
    if (!badPin(pin_num)) bitWrite(pinMap[pin_num].port, pinMap[pin_num].bit, value_for_bit);
}

/* toggle pin number  */
static inline __attribute__((always_inline))
void digitalToggle(uint8_t pin_num) {
    if (!badPin(pin_num)) 
    {
        // Ckeck if pin is in OUTPUT mode befor changing it
        if( ( ( (*pinMap[pin_num].ddr) >> pinMap[pin_num].bit ) & 1) == OUTPUT )  
        {
            digitalWrite(pin_num, !digitalRead(pin_num));
        }
    }
}

/* set pin mode INPUT and OUTPUT */
static inline __attribute__((always_inline))
void pinMode(uint8_t pin_num, bool output_mode) {
    if (!badPin(pin_num)) bitWrite(pinMap[pin_num].ddr, pinMap[pin_num].bit, output_mode);
}

#endif  // DigitalPin_h

