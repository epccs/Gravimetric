/*
Battery application is a CLI  that demonstrates battery charging from photovoltaic on the alternat power input.
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
#include "../lib/timers_bsd.h"
#include "../lib/adc_bsd.h"
#include "../lib/twi0_bsd.h"
#include "../lib/rpu_mgr.h"
#include "../lib/io_enum_bsd.h"
#include "../Uart/id.h"
#include "../Adc/analog.h"
#include "../DayNight/day_night.h"
#include "battery.h"

#define ADC_DELAY_MILSEC 50UL
static unsigned long adc_started_at;

#define DAYNIGHT_BLINK 500UL
static unsigned long daynight_status_blink_started_at;

#define BLINK_DELAY 1000UL
static unsigned long blink_started_at;
static char rpu_addr;
static uint8_t rpu_addr_is_fake;

#define I2C_INTERLEAVE_DELAY 1000UL
static unsigned long last_interleave_started_at;
static uint8_t interleave_i2c;

void ProcessCmd()
{ 
    if ( (strcmp_P( command, PSTR("/id?")) == 0) && ( (arg_count == 0) || (arg_count == 1)) )
    {
        Id("Alternat"); 
    }
    if ( (strcmp_P( command, PSTR("/analog?")) == 0) && ( (arg_count >= 1 ) && (arg_count <= 5) ) )
    {
        Analog(5000UL); 
    }
    if ( (strcmp_P( command, PSTR("/day?")) == 0) && ( (arg_count == 0 ) ) )
    {
        Day(5000UL); 
    }
    if ( (strcmp_P( command, PSTR("/alt")) == 0) && ( (arg_count == 0 ) ) )
    {
        EnableAlt(); 
    }
    if ( (strcmp_P( command, PSTR("/altcntl?")) == 0) && ( (arg_count == 0 ) ) )
    {
        AltPwrCntl(5000UL);  
    }
}

void setup(void) 
{
    ioDir(MCU_IO_CS0_EN, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS0_EN, LOGIC_LEVEL_HIGH);

    ioDir(MCU_IO_CS1_EN, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_CS1_EN, LOGIC_LEVEL_HIGH);

    // Initialize Timers, ADC, and clear bootloader, Arduino does these with init() in wiring.c
    initTimers(); //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.
    init_ADC_single_conversion(EXTERNAL_AVCC); // warning AREF must not be connected to anything
    uart0_init(0,0); // bootloader may have the UART enabled, a zero baudrate will disconnect it.

    // put ADC in Auto Trigger mode and fetch an array of channels
    enable_ADC_auto_conversion(BURST_MODE);
    adc_started_at = milliseconds();

    /* Initialize UART to 38.4kbps, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stderr = stdout = stdin = uart0_init(38400UL, UART0_RX_REPLACE_CR_WITH_NL);
    
    /* Initialize I2C */
    twi0_init(100000UL, TWI0_PINS_PULLUP);

    /* Clear and setup the command buffer, (probably not needed at this point) */
    initCommandBuffer();

    // Enable global interrupts to start TIMER0 and UART ISR's
    sei(); 
    
    blink_started_at = milliseconds();
    daynight_status_blink_started_at = milliseconds();
    
     // manager will broadcast normal mode on DTR pair of mulit-drop
    rpu_addr = i2c_get_Rpu_address(); 
    
    // default address, since RPU manager not found
    if (rpu_addr == 0)
    {
        rpu_addr = '0';
        rpu_addr_is_fake = 1;
    }

    // managers default debounce is 20 min (e.g. 1,200,000 milliseconds) but to test this I want less
    i2c_ul_access_cmd(EVENING_DEBOUNCE,18000UL); // 18 sec is used if it is valid
    i2c_ul_access_cmd(MORNING_DEBOUNCE,18000UL);

    // ALT_V reading of analogRead(ALT_V)*5.0/1024.0*(11/1) where 40 is about 2.1V
    // 80 is about 4.3V, 160 is about 8.6V, 320 is about 17.18V
    // manager uses int bellow and analogRead(ALT_V) to check threshold. 
    i2c_int_access_cmd(EVENING_THRESHOLD,40);
    i2c_int_access_cmd(MORNING_THRESHOLD,80);
}

void blink_mgr_status(void)
{
    unsigned long kRuntime = elapsed(&blink_started_at);

    // normal, all is fine
    if ( kRuntime > BLINK_DELAY)
    {
        ioToggle(MCU_IO_CS0_EN);
        
        // next toggle 
        blink_started_at += BLINK_DELAY; 
    }

    // blink fast if address is fake
    if ( rpu_addr_is_fake && (kRuntime > (BLINK_DELAY/4) ) )
    {
        ioToggle(MCU_IO_CS0_EN);
        
        // set for next toggle 
        blink_started_at += BLINK_DELAY/4; 
    }

    // blink very fast if manager_status bits 0 or 2 (DTR), 1 (twi), 6 (daynight) are set
    if ( (manager_status & ((1<<0) | (1<<1) | (1<<2) | (1<<6)) ) && \
        (kRuntime > (BLINK_DELAY/8) ) )
    {
        ioToggle(MCU_IO_CS0_EN);
        
        // set for next toggle 
        blink_started_at += BLINK_DELAY/8; 
    }
}

void blink_daynight_state(void)
{
    unsigned long kRuntime = elapsed(&daynight_status_blink_started_at);
    uint8_t state = daynight_state;
    if ( ( (state == DAYNIGHT_DAY_STATE) ) && \
        (kRuntime > (DAYNIGHT_BLINK) ) )
    {
        ioWrite(MCU_IO_CS1_EN, LOGIC_LEVEL_HIGH);
     }
    if ( ( (state == DAYNIGHT_NIGHT_STATE) ) && \
        (kRuntime > (DAYNIGHT_BLINK) ) )
    {
        ioWrite(MCU_IO_CS1_EN, LOGIC_LEVEL_LOW);
    }
    if ( ( (state == DAYNIGHT_EVENING_DEBOUNCE_STATE) || \
        (state == DAYNIGHT_MORNING_DEBOUNCE_STATE) ) && \
        (kRuntime > (DAYNIGHT_BLINK/2) ) )
    {
        ioToggle(MCU_IO_CS1_EN);
        
        // set for next toggle 
        daynight_status_blink_started_at += DAYNIGHT_BLINK/2; 
    }
    if ( ( (state == DAYNIGHT_FAIL_STATE) ) && \
        (kRuntime > (DAYNIGHT_BLINK/8) ) )
    {
        ioToggle(MCU_IO_CS1_EN);
        
        // set for next toggle 
        daynight_status_blink_started_at += DAYNIGHT_BLINK/8; 
    }
}

// don't let i2c cause long delays, thus interleave the dispatch of i2c based updates 
void i2c_update_interleave(void)
{
    unsigned long kRuntime = elapsed(&last_interleave_started_at);
    if (kRuntime > (I2C_INTERLEAVE_DELAY) )
    {
        switch (interleave_i2c)
        {
            case 0: 
                check_daynight_state();
                interleave_i2c = 1;
                break;
            default: 
                check_manager_status(); 
                interleave_i2c = 0;
        } 
        last_interleave_started_at += I2C_INTERLEAVE_DELAY;
    }
}

void adc_burst(void)
{
    unsigned long kRuntime= elapsed(&adc_started_at);
    if ((kRuntime) > ((unsigned long)ADC_DELAY_MILSEC))
    {
        enable_ADC_auto_conversion(BURST_MODE);
        adc_started_at += ADC_DELAY_MILSEC; 
    } 
}

int main(void) 
{
    setup();

    while(1) 
    { 
        // use LED to show manager status
        blink_mgr_status();

        // use LED to show daynight_state
        blink_daynight_state();

        // delay between ADC burst
        adc_burst();

        // cordinate i2c updates
        i2c_update_interleave();

        // check if character is available to assemble a command, e.g. non-blocking
        if ( (!command_done) && uart0_available() ) // command_done is an extern from parse.h
        {
            // get a character from stdin and use it to assemble a command
            AssembleCommand(getchar());

            // address is an ascii value, warning: a null address would terminate the command string. 
            StartEchoWhenAddressed(rpu_addr);
        }
        
        // check if a character is available, and if so flush transmit buffer and stop any command that is running.
        // A multi-drop bus can have another device start transmitting after getting an address byte so
        // the first byte is used as a warning, it is the onlly chance to detect a possible collision.
        if ( command_done && uart0_available() )
        {
            // dump the transmit buffer to limit a collision 
            uart0_empty(); 
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
