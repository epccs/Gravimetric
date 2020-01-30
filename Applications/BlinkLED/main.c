/* Blink LED
Copyright (C) 2017 Ronald Sutherland

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES 
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE 
FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY 
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, 
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, 
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Note the library files are LGPL, e.g., you need to publish changes of them but can derive from this 
source and copyright or distribute as you see fit (it is Zero Clause BSD).

https://en.wikipedia.org/wiki/BSD_licenses#0-clause_license_(%22Zero_Clause_BSD%22)
 */ 

#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>
#include "../lib/timers.h"
#include "../lib/uart0.h"
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

    /* Initialize UART to 38.4kbps, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stderr = stdout = stdin = uart0_init(38400UL);

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
    // empty the UART befor halt
    uart0_empty();
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
        if(uart0_available())
        {
            int input = getchar(); // standard C that gets a byte from stdin, which was redirected from the UART
            if (input == '$') 
            {
                printf_P(PSTR("{\"abort\":\"egg found\"}\r\n")); 
                abort_safe();
            }
            printf("%c\r\n", input); //stdout was redirected to UART0
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

