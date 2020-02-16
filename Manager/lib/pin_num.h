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
 
   inline functions are inspired by William Greiman 
   some functions are inspired by Wiring from Hernando Barragan
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

#if defined(__AVR_ATmega328PB__)

#define NUM_DIGITAL_PINS 24
static const Pin_Map pinMap[NUM_DIGITAL_PINS] = {
    [0] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC0 }, // ALT_I
    [1] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC1 }, // ALT_V
    [2] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC2 }, // TX_nRE
    [3] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC3 }, // RX_DE
    [4] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC4 }, // SDA0
    [5] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC5 }, // SCL0
    [6] = { .ddr=&DDRE, .pin=&PINE, .port=&PORTE, .bit= PE2 }, // PWR_I
    [7] = { .ddr=&DDRE, .pin=&PINE, .port=&PORTE, .bit= PE3 }, // PWR_V
    [8] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB0 }, // SHUTDOWN 
    [9] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB1 }, // PIPWR_EN
    [10] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB2 }, // MGR_nSS
    [11] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB3 }, // MGR_MOSI/ALT_EN
    [12] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB4 }, // MGR_MISO
    [13] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB5 }, // MGR_SCK/LED_BUILTIN
    [14] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD0 }, // DTR_RXD
    [15] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD1 }, // DTR_TXD
    [16] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD2 }, // HOST_nCTS
    [17] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD3 }, // HOST_nRTS
    [18] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD4 }, // RX_nRE
    [19] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD5 }, // TX_DE
    [20] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD6 }, // DTR_nRE
    [21] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD7 }, // DTR_DE
    [22] = { .ddr=&DDRE, .pin=&PINE, .port=&PORTE, .bit= PE0 }, // SDA1
    [23] = { .ddr=&DDRE, .pin=&PINE, .port=&PORTE, .bit= PE1 } // SCL1
};
#endif

// note: the use of dead code elimination tricks is not standard C. 
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

