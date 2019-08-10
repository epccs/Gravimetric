/* Blink LED
Copyright (C) 2017 Ronald Sutherland

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

For a copy of the GNU General Public License use
http://www.gnu.org/licenses/gpl-2.0.html
 */ 

#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>
#include "../lib/timers.h"
#include "../lib/uart.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"

// 22mA current sources enabled with CS0_EN and CS1_EN which are defined in ../lib/pins_board.h
#define STATUS_LED CS0_EN

#define BLINK_DELAY 1000UL
static unsigned long blink_started_at;

static int got_a;

void setup(void) 
{
    pinMode(STATUS_LED,OUTPUT);
    digitalWrite(STATUS_LED,HIGH);

    /* Initialize UART, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stderr = stdout = stdin = uartstream0_init(BAUD);

    //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.
    initTimers(); 

    sei(); // Enable global interrupts to start TIMER0
    
    blink_started_at = millis();
    
    got_a = 0;
}

// don't block (e.g. _delay_ms(1000) ), just ckeck if it is time to toggle 
void blink(void)
{
    unsigned long kRuntime = millis() - blink_started_at;
    if ( kRuntime > BLINK_DELAY)
    {
        digitalToggle(STATUS_LED);
        
        // next toggle 
        blink_started_at += BLINK_DELAY; 
    }
}

// abort++. 
void abort_safe(void)
{
    // make sure pins are safe befor waiting on UART 
    pinMode(STATUS_LED,OUTPUT);
    digitalWrite(STATUS_LED,LOW);
    // wait for the UART to finish
    while (uart0_availableForWrite() != UART_TX0_BUFFER_SIZE );
    // turn off interrupts and then loop on LED toggle 
    cli();
    while(1) 
    {
        _delay_ms(100); 
        digitalToggle(STATUS_LED);
    }
}

int main(void)
{
    setup(); 
    
    int abort_yet = 0;
    
    while (1) 
    {
        if(uart0_available()) // refer to core file in ../lib/uart.c
        {
            int input = getchar(); // standard C that gets a byte from stdin, which was redirected from the UART
            if (input == '$') 
            {
                printf_P(PSTR("{\"abort\":\"egg found\"}\r\n")); 
                abort_safe();
            }
            printf("%c\r\n", input); //standard C that sends the byte back to stdout which was redirected to the UART
            if(input == 'a') // a will stop blinking.
            {
                got_a = 1; 
                ++abort_yet; 
            }
            else
            {
              got_a = 0;
            }
            if (abort_yet >= 5) 
            {
                abort_safe();
            }
        }
        if (!got_a)
        {
            blink();
        }
    }    
}

