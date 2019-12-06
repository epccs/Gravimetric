#ifndef Dtr_transmition_H
#define Dtr_transmition_H

#define UART_TTL 500

extern unsigned long uart_started_at;

volatile extern uint8_t uart_output;

// rpubus mode setup
extern void check_DTR(void);
extern void check_uart(void);


#endif // Dtr_transmition_H 
