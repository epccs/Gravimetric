/*
DayNight is a command line controled demonstration of how photovoltaic voltage on ALT_V can be usd to track the day and night.
Copyright (C) 2016 Ronald Sutherland

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
#include <util/delay.h>
#include "../lib/uart0.h"
#include "../lib/parse.h"
#include "../lib/timers.h"
#include "../lib/adc.h"
#include "../lib/twi0.h"
#include "../lib/rpu_mgr.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"
#include "../Uart/id.h"
#include "day_night.h"

#define ADC_DELAY_MILSEC 50UL
static unsigned long adc_started_at;

//pins are defined in ../lib/pins_board.h
#define STATUS_LED CS0_EN
#define DAYNIGHT_STATUS_LED CS1_EN
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
        Id("DayNight"); // ../Uart/id.c
    }
    if ( (strcmp_P( command, PSTR("/day?")) == 0) && ( (arg_count == 0 ) ) )
    {
        Day(5000UL); // day_night.c: show every 5 sec until terminated
    }
}

//At start of each day do this
void day_work(void)
{
    // This shows where to place a task to run when the daynight_state changes to DAYNIGHT_WORK_STATE
    printf_P(PSTR("Day: Charge the battry\r\n")); // manager does this for you
    printf_P(PSTR("        WaterTheGarden\r\n")); // your applicaion code would do this
    return;
}

//At start of each night do this
void night_work(void)
{
    // This shows where to place a task to run when the daynight_state changes to DAYNIGHT_WORK_STATE
    printf_P(PSTR("Night: Block PV caused night current loss\r\n")); // manager does this for you
    printf_P(PSTR("          TurnOnLED's\r\n")); // your applicaion code would do this
    return;
}

void setup(void) 
{
    pinMode(STATUS_LED,OUTPUT);
    digitalWrite(STATUS_LED,HIGH);
    
    pinMode(DAYNIGHT_STATUS_LED,OUTPUT);
    digitalWrite(DAYNIGHT_STATUS_LED,HIGH);
    
    // Initialize Timers, ADC, and clear bootloader, Arduino does these with init() in wiring.c
    initTimers(); //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.
    init_ADC_single_conversion(EXTERNAL_AVCC); // warning AREF must not be connected to anything
    uart0_init(0,0); // bootloader may have the UART enabled, a zero baudrate will disconnect it.

    // put ADC in Auto Trigger mode and fetch an array of channels
    enable_ADC_auto_conversion(BURST_MODE);
    adc_started_at = millis();

    /* Initialize UART to 38.4kbps, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stderr = stdout = stdin = uart0_init(38400UL, UART0_RX_REPLACE_CR_WITH_NL);
    
    /* Clear and setup the command buffer, (probably not needed at this point) */
    initCommandBuffer();

    // manager delays (blocks) for 50mSec after power up so i2c is not running yet
    _delay_ms(60); 

    /* Initialize I2C. note: I2C scan will stop without a pull-up on the bus */
    twi0_init(TWI_PULLUP);

    // Enable global interrupts to start TIMER0 and UART ISR's
    sei(); 

    blink_started_at = millis();
    daynight_status_blink_started_at = millis();
    last_interleave_started_at = millis();

    // manager will broadcast normal mode on DTR pair of mulit-drop
    rpu_addr = i2c_get_Rpu_address(); 
    
    // default address, since RPU manager not found
    if (rpu_addr == 0)
    {
        rpu_addr = '0';
        rpu_addr_is_fake = 1;
    }
    
    // managers default debounce is 20 min (e.g. 1,200,000 millis) but to test this I want less
    i2c_ul_access_cmd(EVENING_DEBOUNCE,18000UL); // 18 sec is used if it is valid
    i2c_ul_access_cmd(MORNING_DEBOUNCE,18000UL);

    // ALT_V reading of analogRead(ALT_V)*5.0/1024.0*(11/1) where 40 is about 2.1V
    // 80 is about 4.3V, 160 is about 8.6V, 320 is about 17.18V
    // manager uses int bellow and analogRead(ALT_V) to check threshold. 
    i2c_int_access_cmd(EVENING_THRESHOLD,40);
    i2c_int_access_cmd(MORNING_THRESHOLD,80);

    // register callback(s) to do the work.
    Day_AttachWork(day_work);
    Night_AttachWork(night_work);
}

void blink_mgr_status(void)
{
    unsigned long kRuntime = millis() - blink_started_at;

    // normal, all is fine
    if ( kRuntime > BLINK_DELAY)
    {
        digitalToggle(STATUS_LED);
        
        // next toggle 
        blink_started_at += BLINK_DELAY; 
    }

    // blink fast if address is fake
    if ( rpu_addr_is_fake && (kRuntime > (BLINK_DELAY/4) ) )
    {
        digitalToggle(STATUS_LED);
        
        // set for next toggle 
        blink_started_at += BLINK_DELAY/4; 
    }

    // blink very fast if manager_status bits 0 or 2 (DTR), 1 (twi), 6 (daynight) are set
    if ( (manager_status & ((1<<0) | (1<<1) | (1<<2) | (1<<6)) ) && \
        (kRuntime > (BLINK_DELAY/8) ) )
    {
        digitalToggle(STATUS_LED);
        
        // set for next toggle 
        blink_started_at += BLINK_DELAY/8; 
    }
}

void blink_daynight_state(void)
{
    unsigned long kRuntime = millis() - daynight_status_blink_started_at;
    uint8_t state = daynight_state;
    if ( ( (state == DAYNIGHT_DAY_STATE) ) && \
        (kRuntime > (DAYNIGHT_BLINK) ) )
    {
        digitalWrite(DAYNIGHT_STATUS_LED,HIGH);
     }
    if ( ( (state == DAYNIGHT_NIGHT_STATE) ) && \
        (kRuntime > (DAYNIGHT_BLINK) ) )
    {
        digitalWrite(DAYNIGHT_STATUS_LED,LOW);
    }
    if ( ( (state == DAYNIGHT_EVENING_DEBOUNCE_STATE) || \
        (state == DAYNIGHT_MORNING_DEBOUNCE_STATE) ) && \
        (kRuntime > (DAYNIGHT_BLINK/2) ) )
    {
        digitalToggle(DAYNIGHT_STATUS_LED);
        
        // set for next toggle 
        daynight_status_blink_started_at += DAYNIGHT_BLINK/2; 
    }
    if ( ( (state == DAYNIGHT_FAIL_STATE) ) && \
        (kRuntime > (DAYNIGHT_BLINK/8) ) )
    {
        digitalToggle(DAYNIGHT_STATUS_LED);
        
        // set for next toggle 
        daynight_status_blink_started_at += DAYNIGHT_BLINK/8; 
    }
}

// don't let i2c cause long delays, thus interleave the dispatch of i2c based updates 
void i2c_update_interleave(void)
{
    unsigned long kRuntime = millis() - last_interleave_started_at;
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
    unsigned long kRuntime= millis() - adc_started_at;
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

        // use LED to show daynight state
        blink_daynight_state();

        // delay between ADC burst
        adc_burst();

        // cordinate i2c updates
        i2c_update_interleave();

        // check if character is available to assemble a command, e.g. non-blocking
        if ( (!command_done) && uart0_available() ) // command_done is an extern from parse.h
        {
            // get a character from stdin and use it to assemble a command
            AssembleCommand(getchar()); // ../lib/parse.c

            // address is an ascii value, warning: a null address would terminate the command string. 
            StartEchoWhenAddressed(rpu_addr); // ../lib/parse.c
        }

        // check if a character is available, and if so flush transmit buffer and nuke the command in process.
        // A multi-drop bus can have another device start transmitting after getting an address byte so
        // the first byte is used as a warning, it is the onlly chance to detect a possible collision.
        if ( command_done && uart0_available() )
        {
            // dump the transmit buffer to limit a collision 
            uart0_flush();  // ../lib/uart.c
            initCommandBuffer();
        }

        // finish echo of the command line befor starting a reply (or the next part of a reply)
        if ( command_done && uart0_availableForWrite() )
        {
            if ( !echo_on  )
            { // this happons when the address did not match 
                initCommandBuffer(); // ../lib/parse.c
            }
            else
            {
                if (command_done == 1) 
                { 
                    findCommand(); // ../lib/parse.c
                    // steps 2..9 are skipped. Reserved for more complex parse
                    command_done = 10;
                }

                // do not overfill the serial buffer since that blocks looping, e.g. process a command in 32 byte chunks
                if ( (command_done >= 10) && (command_done < 250) )
                {
                    // setps 10..249 are moved through by the procedure selected
                     ProcessCmd();
                }
                else 
                {
                    initCommandBuffer(); // ../lib/parse.c
                }
            }
        }
    }        
    return 0;
}
