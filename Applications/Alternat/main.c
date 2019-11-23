/*
Alternat is a CLI demonstration to show Alternat power input sending photovoltaic power to a battery.
Copyright (C) 2018 Ronald Sutherland

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
#include <util/atomic.h>
#include "../lib/uart.h"
#include "../lib/parse.h"
#include "../lib/timers.h"
#include "../lib/adc.h"
#include "../lib/twi0.h"
#include "../lib/rpu_mgr.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"
#include "../Uart/id.h"
#include "../Adc/analog.h"
//#include "../DayNight/day_night.h"
#include "alternat.h"

#define ADC_DELAY_MILSEC 50UL
static unsigned long adc_started_at;

#define STATUS_LED CS0_EN

#define DAYNIGHT_STATUS_LED CS1_EN
#define DAYNIGHT_BLINK 500UL
static unsigned long day_status_blink_started_at;

#define BLINK_DELAY 1000UL
static unsigned long blink_started_at;
static unsigned long blink_delay;
static char rpu_addr;

void ProcessCmd()
{ 
    if ( (strcmp_P( command, PSTR("/id?")) == 0) && ( (arg_count == 0) || (arg_count == 1)) )
    {
        Id("Alternat"); // ../Uart/id.c
    }
    if ( (strcmp_P( command, PSTR("/analog?")) == 0) && ( (arg_count >= 1 ) && (arg_count <= 5) ) )
    {
        Analog(20000UL); // ../Adc/analog.c: show every 20 sec until terminated
    }
    if ( (strcmp_P( command, PSTR("/day?")) == 0) && ( (arg_count == 0 ) ) )
    {
        //Day(60000UL); // ../DayNight/day_night.c: show every 60 sec until terminated
    }
    if ( (strcmp_P( command, PSTR("/alt")) == 0) && ( (arg_count == 0 ) ) )
    {
        EnableAlt(); // alternat.c
    }
    if ( (strcmp_P( command, PSTR("/altcnt?")) == 0) && ( (arg_count == 0 ) ) )
    {
        //AltCount(); // alternat.c
    }
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
    init_uart0_after_bootloader(); // bootloader may have the UART setup

    // put ADC in Auto Trigger mode and fetch an array of channels
    enable_ADC_auto_conversion(BURST_MODE);
    adc_started_at = millis();

    /* Initialize UART, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stdout = stdin = uartstream0_init(BAUD);
    
    /* Initialize I2C. note: I2C scan will stop without a pull-up on the bus */
    twi0_init(TWI_PULLUP);

    /* Clear and setup the command buffer, (probably not needed at this point) */
    initCommandBuffer();

    // Enable global interrupts to start TIMER0 and UART ISR's
    sei(); 
    
    blink_started_at = millis();
    day_status_blink_started_at = millis();
    
    rpu_addr = i2c_get_Rpu_address();
    blink_delay = BLINK_DELAY;
    
    // blink fast if a default address from RPU manager not found
    if (rpu_addr == 0)
    {
        rpu_addr = '0';
        blink_delay = BLINK_DELAY/4;
    }

    
    // default debounce is 15 min (e.g. 900,000 millis)
    // evening_debouce = 18000UL; // 18 sec
    // morning_debouce = 18000UL;
    // ALT_V reading of 40*5.0/1024.0*(11/1) is about 2.1V
    // ALT_V reading of 80*5.0/1024.0*(11/1) is about 4.3V
    // ALT_V reading of 160*5.0/1024.0*(11/1) is about 8.6V
    // ALT_V reading of 320*5.0/1024.0*(11/1) is about 17.18V
    // evening_threshold = 40; 
    // morning_threshold = 80;
}

void blink(void)
{
    unsigned long kRuntime = millis() - blink_started_at;
    if ( kRuntime > blink_delay)
    {
        digitalToggle(STATUS_LED);
        
        // next toggle 
        blink_started_at += blink_delay; 
    }
}

void blink_day_status(void)
{
    unsigned long kRuntime = millis() - day_status_blink_started_at;
    uint8_t state = DayState();
    if ( ( (state == DAYNIGHT_EVENING_DEBOUNCE_STATE) || \
           (state == DAYNIGHT_MORNING_DEBOUNCE_STATE) ) && \
           (kRuntime > (DAYNIGHT_BLINK/2) ) )
    {
        digitalToggle(DAYNIGHT_STATUS_LED);
        
        // set for next toggle 
        day_status_blink_started_at += DAYNIGHT_BLINK/2; 
    }
    if ( ( (state == DAYNIGHT_DAY_STATE) ) && \
           (kRuntime > (DAYNIGHT_BLINK) ) )
    {
        digitalWrite(DAYNIGHT_STATUS_LED,HIGH);
        
        // set for next toggle 
        day_status_blink_started_at += DAYNIGHT_BLINK; 
    }
    if ( ( (state == DAYNIGHT_NIGHT_STATE) ) && \
           (kRuntime > (DAYNIGHT_BLINK) ) )
    {
        digitalWrite(DAYNIGHT_STATUS_LED,LOW);
        
        // set for next toggle 
        day_status_blink_started_at += DAYNIGHT_BLINK; 
    }
    if ( ( (state == DAYNIGHT_FAIL_STATE) ) && \
           (kRuntime > (DAYNIGHT_BLINK/8) ) )
    {
        digitalToggle(DAYNIGHT_STATUS_LED);
        
        // set for next toggle 
        day_status_blink_started_at += DAYNIGHT_BLINK/8; 
    }
}

uint8_t adc_burst(void)
{
    unsigned long kRuntime= millis() - adc_started_at;
    if ((kRuntime) > ((unsigned long)ADC_DELAY_MILSEC))
    {
        enable_ADC_auto_conversion(BURST_MODE);
        adc_started_at += ADC_DELAY_MILSEC; 
        return 1;
    } 
    else
    {
        return 0;
    }
}

int main(void) 
{
    setup();

    while(1) 
    { 
        // use STATUS_LED to show if I2C has a bus manager
        blink();

        // use DAYNIGHT_STATUS_LED to show day_state
        blink_day_status();

        // delay between ADC burst
        if ( adc_burst() )
        {
            // Check Day day-night state machine.

        }

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
            uart0_flush(); 
            initCommandBuffer();
        }
        
        // finish echo of the command line befor starting a reply (or the next part of a reply)
        if ( command_done && (uart0_availableForWrite() == UART_TX0_BUFFER_SIZE) )
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
