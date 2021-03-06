/*
i2c-debug is a command line controled demonstration of the 2-wire Serial Interface
Copyright (C) 2016 Ronald Sutherland

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES 
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE 
FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY 
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, 
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, 
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

https://en.wikipedia.org/wiki/BSD_licenses#0-clause_license_(%22Zero_Clause_BSD%22)
*/

#include <stdbool.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <util/delay.h>
#include "../lib/parse.h"
#include "../lib/uart0_bsd.h"
#include "../lib/parse.h"
#include "../lib/timers_bsd.h"
#include "../lib/twi0_bsd.h"
#include "../lib/rpu_mgr.h"
#include "../lib/io_enum_bsd.h"
#include "../Uart/id.h"
#include "i2c0-scan.h"
#include "i2c0-cmd.h"
#include "i2c0-monitor.h"

#define BLINK_DELAY 1000UL
static unsigned long blink_started_at;
static unsigned long blink_delay;
static char rpu_addr;

void ProcessCmd()
{ 
    if ( (strcmp_P( command, PSTR("/id?")) == 0) && ( (arg_count == 0) || (arg_count == 1)) )
    {
        Id("I2C0debug^2");
    }
    if ( (strcmp_P( command, PSTR("/iscan?")) == 0) && (arg_count == 0) )
    {
        I2c0_scan();
    }
    if ( (strcmp_P( command, PSTR("/iaddr")) == 0) && (arg_count == 1) )
    {
        I2c0_address();
    }
    if ( (strcmp_P( command, PSTR("/ibuff")) == 0) )
    {
        I2c0_txBuffer();
    }
    if ( (strcmp_P( command, PSTR("/ibuff?")) == 0) && (arg_count == 0) )
    {
        I2c0_txBuffer();
    }
    if ( (strcmp_P( command, PSTR("/iwrite")) == 0) && (arg_count == 0) )
    {
        I2c0_write();
    }
    if ( (strcmp_P( command, PSTR("/iread?")) == 0) && (arg_count == 1) )
    {
        I2c0_read();
    }
    if ( (strcmp_P( command, PSTR("/imon?")) == 0) && (arg_count == 1) )
    {
        I2c0_monitor();
    }
}

void setup(void) 
{
    // Status LED
    ioDir(MCU_IO_CS0_EN, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS0_EN, LOGIC_LEVEL_HIGH);
    
  // Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.
    initTimers(); 
    uart0_init(0,0); // bootloader may have the UART enabled, a zero baudrate will disconnect it.
    
    /* Initialize UART to 38.4kbps, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stderr = stdout = stdin = uart0_init(38400UL, UART0_RX_REPLACE_CR_WITH_NL);

    // manager delays (blocks) for 50mSec after power up so i2c is not running yet
    _delay_ms(60); 

    /* Initialize I2C. note: an I2C scan will stop without a pull-up on the bus */
    twi0_init(100000UL, TWI0_PINS_PULLUP);

    /* Clear and setup the command buffer, (probably not needed at this point) */
    initCommandBuffer();

    // Enable global interrupts to start TIMER0 and UART ISR's
    sei(); 

    blink_started_at = milliseconds();
    
    rpu_addr = i2c_get_Rpu_address();
    blink_delay = BLINK_DELAY;
    
    // blink fast if a default address from RPU manager not found
    if (rpu_addr == 0)
    {
        rpu_addr = '0';
        blink_delay = BLINK_DELAY/4;
    }
}

// Status LED
void blink(void)
{
    unsigned long kRuntime = elapsed(&blink_started_at);
    if ( kRuntime > blink_delay)
    {
        ioToggle(MCU_IO_CS0_EN);
        
        // next toggle 
        blink_started_at += blink_delay;
    }
}

int main(void) 
{    
    setup(); 

    while(1) 
    {
        // use LED to see if I2C has a bus manager
        blink();

        // check if character is available to assemble a command, e.g. non-blocking
        if ( (!command_done) && uart0_available() ) // command_done is an extern from parse.h
        {
            // get a character from stdin and use it to assemble a command
            AssembleCommand(getchar());

            // address is the ascii value for '0' note: a null address will terminate the command string. 
            StartEchoWhenAddressed(rpu_addr);
        }
        
        // check if a character is available, and if so flush transmit buffer and nuke the command in process.
        // A multi-drop bus can have another device start transmitting after getting an address byte so
        // the first byte is used as a warning, it is the onlly chance to detect a possible collision.
        if ( command_done && uart0_available() )
        {
            // dump the transmit buffer to limit a collision 
            uart0_empty();

            // turn off the slave monitor
            twi0_slaveAddress(0);

            initCommandBuffer();
        }
        
        // finish echo of the command line befor starting a reply (or the next part of a reply)
        if ( command_done && uart0_availableForWrite() )
        {
            if ( !echo_on  )
            { // this happons when the address did not match 
                initCommandBuffer();
            }
            else
            {
                if (command_done == 1)  
                {
                    findCommand();
                    command_done = 10;
                }
                
                // do not overfill the serial buffer since that blocks looping, e.g. process a command in 32 byte chunks
                if ( (command_done >= 10) && (command_done < 250) )
                {
                     ProcessCmd();
                }
                else 
                {
                    initCommandBuffer();
                }
            }
         }
    }        
    return 0;
}
