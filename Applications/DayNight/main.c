/*
DayNight is a command line controled demonstration of how photovoltaic voltage on ALT_V can be usd to track the day and night.
Copyright (C) 2016 Ronald Sutherland

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
#include "day_night.h"

#define ADC_DELAY_MILSEC 200UL
static unsigned long adc_started_at;

//pins are defined in ../lib/pins_board.h
#define STATUS_LED CS0_EN
#define DAYNIGHT_STATUS_LED CS1_EN
#define DAYNIGHT_BLINK 500UL
static unsigned long daynight_status_blink_started_at;
static unsigned long daynight_status_checked_at;

#define BLINK_DELAY 1000UL
static unsigned long blink_started_at;
static unsigned long blink_delay;
static char rpu_addr;

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
    printf_P(PSTR("Day: Charge the battry\r\n")); // manager does this
    printf_P(PSTR("        WaterTheGarden\r\n")); // used for debuging
    return;
}

//At start of each night do this
void night_work(void)
{
    // This shows where to place a task to run when the daynight_state changes to DAYNIGHT_WORK_STATE
    printf_P(PSTR("Night: Block night current loss into PV\r\n")); // manager does this
    printf_P(PSTR("          TurnOnLED's\r\n")); // used for debuging
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
    daynight_status_blink_started_at = millis();
    
    rpu_addr = i2c_get_Rpu_address();
    blink_delay = BLINK_DELAY;
    
    // blink fast if a default address from RPU manager not found
    if (rpu_addr == 0)
    {
        rpu_addr = '0';
        blink_delay = BLINK_DELAY/4;
    }
    
    // managers default debounce is 20 min (e.g. 1,200,000 millis) but to test this I want less
    i2c_ul_access_cmd(EVENING_DEBOUNCE,18000UL); // 18 sec is used if it is valid
    daynight_evening_debounce = i2c_ul_access_cmd(EVENING_DEBOUNCE,0); // non valid data allows checking that it got set
    i2c_ul_access_cmd(MORNING_DEBOUNCE,18000UL);
    daynight_morning_debounce = i2c_ul_access_cmd(MORNING_DEBOUNCE,0);

    // ALT_V reading of analogRead(ALT_V)*5.0/1024.0*(11/1) where 40 is about 2.1V
    // 80 is about 4.3V, 160 is about 8.6V, 320 is about 17.18V
    // manager uses int bellow and analogRead(ALT_V) to check threshold. 
    i2c_int_access_cmd(EVENING_THRESHOLD,40);
    daynight_evening_threshold = i2c_int_access_cmd(EVENING_THRESHOLD,0);
    i2c_int_access_cmd(MORNING_THRESHOLD,80);
    daynight_morning_threshold = i2c_int_access_cmd(MORNING_THRESHOLD,0);
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

void day_status(void)
{
    unsigned long kRuntime = millis() - daynight_status_checked_at;
    if (kRuntime > (DAYNIGHT_BLINK << 1) )
    {
        // Check day-night state machine (once per blink is fine).
        check_daynight_state(); // day_night.c
        daynight_status_checked_at += (DAYNIGHT_BLINK << 1);
    }
    kRuntime = millis() - daynight_status_blink_started_at;
    uint8_t state = daynight_state;
    if ( ( (state == DAYNIGHT_EVENING_DEBOUNCE_STATE) || \
        (state == DAYNIGHT_MORNING_DEBOUNCE_STATE) ) && \
        (kRuntime > (DAYNIGHT_BLINK/2) ) )
    {
        digitalToggle(DAYNIGHT_STATUS_LED);
        
        // set for next toggle 
        daynight_status_blink_started_at += DAYNIGHT_BLINK/2; 
    }
    if ( ( (state == DAYNIGHT_DAY_STATE) ) && \
        (kRuntime > (DAYNIGHT_BLINK) ) )
    {
        digitalWrite(DAYNIGHT_STATUS_LED,HIGH);
        
        // set for next toggle 
        daynight_status_blink_started_at += DAYNIGHT_BLINK; 
    }
    if ( ( (state == DAYNIGHT_NIGHT_STATE) ) && \
        (kRuntime > (DAYNIGHT_BLINK) ) )
    {
        digitalWrite(DAYNIGHT_STATUS_LED,LOW);
        
        // set for next toggle 
        daynight_status_blink_started_at += DAYNIGHT_BLINK; 
    }
    if ( ( (state == DAYNIGHT_FAIL_STATE) ) && \
        (kRuntime > (DAYNIGHT_BLINK/8) ) )
    {
        digitalToggle(DAYNIGHT_STATUS_LED);
        
        // set for next toggle 
        daynight_status_blink_started_at += DAYNIGHT_BLINK/8; 
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
        // use LED to show if I2C has a bus manager
        blink();

        // use LED to show day_state
        day_status();

        // delay between ADC burst
        adc_burst();

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
        if ( command_done && (uart0_availableForWrite() == UART_TX0_BUFFER_SIZE) )
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
