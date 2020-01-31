#ifndef UART0_H
#define UART0_H

// https://www.microchip.com/webdoc/AVRLibcReferenceManual/group__avr__stdio.html
#include <stdio.h>

// Buffer size: (1<<5), (1<<4), (1<<3), (1<<2).
#define UART0_RX0_SIZE (1<<5)
#define UART0_TX0_SIZE (1<<5)

// options
#define UART_TX_REPLACE_NL_WITH_CR 0x01         // replace transmited newline with carriage return
#define UART_RX_REPLACE_CR_WITH_NL 0x02         // replace receive carriage return with newline

// error codes
#define UART_NO_DATA               0x01         // no receive data available
#define UART_BUFFER_OVERFLOW       0x02         // receive ringbuffer overflow
#define UART_OVERRUN_ERROR         0x04         // from USARTn Control and Status Register A bit for Data OverRun (DOR)
#define UART_FRAME_ERROR           0x08         // from USARTn Control and Status Register A bit for Frame Error (FE)

// error codes UART_FRAME_ERROR, UART_OVERRUN_ERROR, UART_BUFFER_OVERFLOW, UART_NO_DATA
extern volatile uint8_t UART0_error;

extern void uart0_flush(void);
extern void uart0_empty(void);
extern uint16_t uart0_available(void);
extern uint16_t uart0_availableForWrite(void);
extern FILE *uart0_init(uint32_t baudrate, uint8_t choices);
extern int uart0_putchar(char c, FILE *stream);
extern int uart0_getchar(FILE *stream);

#endif // UART0_H 
