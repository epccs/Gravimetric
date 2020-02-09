/*
    Interrupt-Driven UART for AVR Standard IO facilities streams 
    Copyright (C) 2020 Ronald Sutherland

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

    API has a few functions like Arduino Serial but is done in C for AVR Standard IO facilities streams
    https://www.microchip.com/webdoc/AVRLibcReferenceManual/group__avr__stdio.html
    
    The standard streams stdin, stdout, and stderr are provided, but contrary to the C standard, 
    since avr-libc has no knowledge about applicable devices, these streams are not already 
    pre-initialized at application startup. Also, since there is no notion of "file" whatsoever to 
    avr-libc, there is no function fopen() that could be used to associate a stream to some device. 
    Instead, the function fdevopen() is provided to associate a stream to a device, where the device 
    needs to provide a function to send a character, to receive a character, or both. There is no 
    differentiation between "text" and "binary" streams inside avr-libc. Character \n is sent literally 
    down to the device's put() function. If the device requires a carriage return (\r) character to be 
    sent before the linefeed, its put() routine must implement this 
    
    UART0_TX_REPLACE_NL_WITH_CR and UART0_RX_REPLACE_CR_WITH_NL may be used 
    to filter data into and out of the uart.

    Register names (RXCIE0) and bits (FE0, DOR0, RXEN0, TXEN0, UCSZ00, UDRIE0) are for ATmega328p[b].
*/

#include <stdio.h>
#include <stdbool.h>
#include <util/atomic.h>
#include "uart0.h"

//  if 0x8000 bit is set then (U2X) Double speed mode is used
#define UART0_BAUD_SELECT(baudRate) ((F_CPU+8UL*(baudRate))/(16UL*(baudRate))-1UL)

static volatile uint8_t TxBuf[UART0_TX0_SIZE];
static volatile uint8_t RxBuf[UART0_RX0_SIZE];
static volatile uint8_t TxHead;
static volatile uint8_t TxTail;
static volatile uint8_t RxHead;
static volatile uint8_t RxTail;

static uint8_t options;
volatile uint8_t UART0_error;

ISR(USART0_RX_vect)
{
    uint16_t next_index;
    uint8_t data;
 
    // check USARTn Control and Status Register A for Frame Error (FE) or Data OverRun (DOR)
    uint8_t last_status = (UCSR0A & ((1<<FE0)|(1<<DOR0)) );

    // above errors are valid until UDR0 is read, e.g., now
    data = UDR0;

    next_index = ( RxHead + 1) & ( UART0_RX0_SIZE - 1);
    
    if ( next_index == RxTail ) 
    {
        last_status += UART0_BUFFER_OVERFLOW;
    } 
    else 
    {
        RxHead = next_index;
        RxBuf[next_index] = data;
    }
    UART0_error = last_status;   
}


ISR(USART0_UDRE_vect)
{
    uint16_t tmptail;

    if ( TxHead != TxTail) 
    {
        tmptail = (TxTail + 1) & ( UART0_TX0_SIZE - 1); // calculate and store new buffer index
        TxTail = tmptail;
        UDR0 = TxBuf[tmptail]; // get one byte from buffer and send it with UART
    } 
    else 
    {
        UCSR0B &= ~(1<<UDRIE0); // tx buffer empty, disable UDRE interrupt
    }
}

// Flush bytes from the transmit buffer with busy waiting, like the Arduino API.
// https://www.arduino.cc/reference/en/language/functions/communication/serial/flush/
void uart0_flush(void)
{
    while (TxHead != TxTail)
    {
        //busy waiting
    };
}

// Immediately stop transmitting by removing any buffered outgoing serial data.
// helps to reduce/avoid collision damage on full-duplex multi-drop
void uart0_empty(void)
{
    TxHead = TxTail;
}

// Number of bytes available in the receive buffer, like the Arduino API.
// https://www.arduino.cc/reference/en/language/functions/communication/serial/available/
int uart0_available(void)
{
    return (UART0_RX0_SIZE + RxHead - RxTail) & ( UART0_RX0_SIZE - 1);
}

// Transmit buffer (all of it) is available for writing without blocking.
bool uart0_availableForWrite(void)
{
    return (TxHead == TxTail);
}

// Protofunctions (code is latter) to allow UART0 to be used as a stream for printf, scanf, etc...
int uart0_putchar(char c, FILE *stream);
int uart0_getchar(FILE *stream);

// Stream declaration for stdio
static FILE uartstream0_f = FDEV_SETUP_STREAM(uart0_putchar, uart0_getchar, _FDEV_SETUP_RW);

// Initialize the UART and return file handle, disconnect UART if baudrate is zero.
// choices e.g., UART0_TX_REPLACE_NL_WITH_CR & UART0_RX_REPLACE_CR_WITH_NL
FILE *uart0_init(uint32_t baudrate, uint8_t choices)
{
    uint16_t ubrr = UART0_BAUD_SELECT(baudrate);

    /* UART: how optiboot does it.
          UCSR0A = _BV(U2X0); //Double speed mode 
          UCSR0B = _BV(RXEN0) | _BV(TXEN0); // enable TX and RX glitch free
          UCSR0C = (1<<UCSZ00) | (1<<UCSZ01); // control frame format
          UBRR0L = (uint8_t)( (F_CPU + BAUD * 4L) / (BAUD * 8L) - 1 );
    */
    
    TxHead = 0;
    TxTail = 0;
    RxHead = 0;
    RxTail = 0;

    if (ubrr & 0x8000) 
    {
        UCSR0A = (1<<U2X0);  //Double speed mode (status register)
        ubrr &= ~0x8000;
    }

    // disconnect UART if baud is zero
    if (ubrr == 0)
    {
        UCSR0B = 0;
    }
    else
    {
        UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0); // enable TX and RX
        UCSR0C = (3<<UCSZ00); // control frame format asynchronous, 8data, no parity, 1stop bit
        UBRR0H = (uint8_t)(ubrr>>8);
        UBRR0L = (uint8_t) ubrr;
    }

    options = choices;

    return &uartstream0_f;
}

int uart0_putchar(char c, FILE *stream)
{
    uint16_t next_index;

    next_index  = (TxHead + 1) & ( UART0_TX0_SIZE - 1);

    while ( next_index == TxTail ) 
    {
        ;// busy wait for free space in buffer
    }

    // I put a carriage return and newline in the printf string  
    // so I don't use UART0_TX_REPLACE_NL_WITH_CR
    if ( (options & UART0_TX_REPLACE_NL_WITH_CR) && (c == '\n') )
    {
        TxBuf[next_index] = (uint8_t)'\r';
    }
    else
    {
        TxBuf[next_index] = (uint8_t) c;
    }
    TxHead = next_index;

    // Data Register Empty Interrupt Enable (UDRIE)
    // When the UDRIE bit in UCSRnB is written to '1', the USART Data Register Empty Interrupt 
    // will be executed as long as UDRE is set (provided that global interrupts are enabled).
    UCSR0B |= (1<<UDRIE0);

    return 0;
}

int uart0_getchar(FILE *stream)
{
    uint16_t next_index;
    uint8_t data;

    while( !(uart0_available()) );  // wait for input

    if ( RxHead == RxTail ) 
    {
        UART0_error += UART0_NO_DATA;
        data = 0;
    }
    else
    {
        next_index = (RxTail + 1) & ( UART0_RX0_SIZE - 1);
        RxTail = next_index;
        data = RxBuf[next_index]; // get byte from rx buffer
    }

    // I use UART0_RX_REPLACE_CR_WITH_NL to simplify command parsing from a host 
    if ( (options & UART0_RX_REPLACE_CR_WITH_NL) && (data == '\r') ) data = '\n';
    return (int) data;
}


