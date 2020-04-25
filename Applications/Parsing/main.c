/*
Demonstration of serial Parsing for textual commands and arguments.
Copyright (C) 2020 Ronald Sutherland

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

#include <stdbool.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include "../lib/uart0_bsd.h"
#include "../lib/parse.h"
#include "../lib/twi0_bsd.h"
#include "../lib/rpu_mgr.h"

static char rpu_addr;

int main(void) {    

    /* Initialize UART to 38.4kbps, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stderr = stdout = stdin = uart0_init(38400UL, UART0_RX_REPLACE_CR_WITH_NL);

    /* Initialize I2C to manager*/
    twi0_init(100000UL, TWI0_PINS_PULLUP);

    initCommandBuffer();

    // Enable global interrupts to start TIMER0 and UART
    sei(); 
    
    rpu_addr = i2c_get_Rpu_address();
    
    // default address if manager not found
    if (rpu_addr == 0)
    {
        rpu_addr = '0';
    }

    // non-blocking code in loop
    while(1) 
    {
        // check if character is available to assemble a command, e.g. non-blocking
        if ( (!command_done) && uart0_available() ) // command_done is an extern from parse.h
        {
            // get a character from stdin and use it to assemble a command
            AssembleCommand(getchar());

            // address is the ascii value for '0' note: a null address will terminate the command string. 
            StartEchoWhenAddressed(rpu_addr);
        }
        
        // check if the character is available, and if so stop transmit and the command in process.
        // a multi-drop bus can have another device start transmitting after the second received byte so
        // there is little time to detect a possible collision
        if ( command_done && uart0_available() )
        {
            // dump the transmit buffer to limit a collision 
            uart0_empty(); 
            initCommandBuffer();
        }

        // finish echo of the command line befor starting a reply (or the next part of reply), also non-blocking.
        if ( command_done && uart0_availableForWrite() )
        {
            if ( !echo_on  )
            { // this happons when the address did not match
                initCommandBuffer();
            }
            else
            {
                // command is a pointer to string and arg[] is an array of pointers to strings
                // use findCommand to make them point to the correct places in the command line
                // this can only be done once, since spaces and delimeters are replaced with null termination
                if (command_done == 1)  
                {
                    findCommand();
                    command_done = 2;
                }
                if (command_done == 2)
                {
                    if (command != '\0' )
                    {
                        printf_P(PSTR("{\"cmd\": \"%s\"}\r\n"),command);
                        command_done = 3;
                    }
                    else
                    {
                        initCommandBuffer();
                    }
                }
                if (command_done == 3)
                {
                    printf_P(PSTR("{\"arg_count\": \"%d\"}\r\n"),arg_count);
                    if (arg_count > 0)
                    {
                        command_done = 4;
                    }
                    else
                    {
                        initCommandBuffer();
                    }
                }
                if (command_done == 4)
                {
                    printf_P(PSTR("{\"arg[0]\": \"%s\"}\r\n"),arg[0]);
                    if (arg_count > 1)
                    {
                        command_done = 5;
                    }
                    else
                    {
                        initCommandBuffer();
                    }
                }
                if (command_done == 5)
                {
                    printf_P(PSTR("{\"arg[1]\": \"%s\"}\r\n"),arg[1]);
                    if (arg_count > 2)
                    {
                        command_done = 6;
                    }
                    else
                    {
                        initCommandBuffer();
                    }
                }
                if (command_done == 6)
                {
                    printf_P(PSTR("{\"arg[2]\": \"%s\"}\r\n"),arg[2]);
                    if (arg_count > 3)
                    {
                        command_done = 7;
                    }
                    else
                    {
                        initCommandBuffer();
                    }
                }
                if (command_done == 7)
                {
                    printf_P(PSTR("{\"arg[3]\": \"%s\"}\r\n"),arg[3]);
                    if (arg_count > 4)
                    {
                        command_done = 8;
                    }
                    else
                    {
                        initCommandBuffer();
                    }
                }
                if (command_done == 8)
                {
                    printf_P(PSTR("{\"arg[4]\": \"%s\"}\r\n"),arg[4]);
                    initCommandBuffer();
                }
            }
         }
    }        
    return 0;
}
