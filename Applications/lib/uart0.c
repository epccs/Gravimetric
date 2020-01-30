/*
    AVR Interrupt-Driven UART
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

    API is somewhat like Arduino Serial but done in C for AVR Standard IO facilities 
    https://www.microchip.com/webdoc/AVRLibcReferenceManual/group__avr__stdio.html
*/

#include <stdio.h>
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

volatile uint8_t UART0_error;

ISR(USART0_RX_vect)
{
    uint16_t next_index;
    uint8_t data;
 
    // check USARTn Control and Status Register A for Frame Error (FE) or Data OverRun (DOR)
    uint8_t last_status = (UCSR0A & ((1<<FE0)|(1<<DOR)) );

    // above errors are valid until UDR0 is read, e.g., now
    data = UDR0;

    next_index = ( RxHead + 1) & ( UART0_RX0_SIZE - 1);
    
    if ( next_index == RxTail ) 
    {
        last_status += UART_BUFFER_OVERFLOW;
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

// Flush bytes from the transmit buffer, like the Arduino API.
// https://www.arduino.cc/reference/en/language/functions/communication/serial/flush/
void uart0_flush(void)
{
    while (TxHead != TxTail)
    {
        //busy waiting
    };
}

// Remove any buffered incoming or outgoing serial data.
void uart0_empty(void)
{
    TxHead = TxTail;
    RxHead = RxTail;
}

// Number of bytes available in the receive buffer, like the Arduino API.
// https://www.arduino.cc/reference/en/language/functions/communication/serial/available/
uint16_t uart0_available(void)
{
    return (UART0_RX0_SIZE + RxHead - RxTail) & ( UART0_RX0_SIZE - 1);
}

// Number of bytes available for writing to the transmit buffer without blocking, like the Arduino API.
// https://www.arduino.cc/reference/en/language/functions/communication/serial/availableforwrite/
uint16_t uart0_availableForWrite(void)
{
    return (UART0_TX0_SIZE - ( (UART0_TX0_SIZE + TxHead - TxTail) & ( UART0_TX0_SIZE - 1) ) );
}

// Protofunctions (code is latter) to allow UART0 to be used as a stream for printf, scanf, etc...
int uart0_putchar(char c, FILE *stream);
int uart0_getchar(FILE *stream);

// Stream declaration for stdio
static FILE uartstream0_f = FDEV_SETUP_STREAM(uart0_putchar, uart0_getchar, _FDEV_SETUP_RW);

// Initialize the UART and return file handle, disconnect UART if baudrate is zero.
FILE *uart0_init(uint32_t baudrate)
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

    if (c == '\n')
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
        UART0_error += UART_NO_DATA;
        data = 0;
    }
    else
    {
        next_index = (RxTail + 1) & ( UART0_RX0_SIZE - 1);
        RxTail = next_index;
        data = RxBuf[next_index]; // get byte from rx buffer
    }
    //if(data == '\r') data = '\n';
    return (int) data;
}


