/*
day-night  is a library that is used to track the day and night for control functions. 
Copyright (C) 2019 Ronald Sutherland

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
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../lib/parse.h"
#include "../lib/timers_bsd.h"
#include "../lib/rpu_mgr.h"
#include "../lib/rpu_mgr_callback.h"
#include "../lib/adc_bsd.h"
#include "day_night.h"
#include "main.h"

volatile uint8_t daynight_state = 0; 
#define CHK_DAYNIGHT_DELAY 2000UL

static unsigned long daynight_serial_print_started_at;


void Day(unsigned long serial_print_delay_milsec)
{
    if ( (command_done == 10) )
    {
        daynight_serial_print_started_at = milliseconds();
        printf_P(PSTR("{\"state\":\"0x%X\","),daynight_state); // print a hex value
        command_done = 12;
    }
    else if ( (command_done == 12) ) 
    {
        int local_copy = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            local_copy = i2c_int_access_cmd(MORNING_THRESHOLD,0,&loop_state);
        }
        printf_P(PSTR("\"mor_threshold\":\"%u\","),local_copy);
        command_done = 13;
    }
    else if ( (command_done == 13) ) 
    {
        int local_copy = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            local_copy = i2c_int_access_cmd(EVENING_THRESHOLD,0,&loop_state);
        }
        printf_P(PSTR("\"eve_threshold\":\"%u\","),local_copy);
        command_done = 14;
    }
    else if ( (command_done == 14) ) 
    {
        int adc_reads = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            adc_reads = i2c_get_adc_from_manager(ADC_CH_MGR_ALT_V,&loop_state);
        }
        printf_P(PSTR("\"adc_alt_v\":\"%u\","),adc_reads);
        command_done = 15;
    }
    else if ( (command_done == 15) ) 
    {
        unsigned long local_copy = i2c_ul_access_cmd(MORNING_DEBOUNCE,0);
        printf_P(PSTR("\"mor_debounce\":\"%lu\","),local_copy);
        command_done = 16;
    }
    else if ( (command_done == 16) ) 
    {
        unsigned long local_copy = i2c_ul_access_cmd(EVENING_DEBOUNCE,0);
        printf_P(PSTR("\"eve_debounce\":\"%lu\","),local_copy);
        command_done = 17;
    }
    else if ( (command_done == 17) ) 
    {
        unsigned long local_copy = i2c_ul_access_cmd(DAYNIGHT_TIMER,0);
        printf_P(PSTR("\"dn_timer\":\"%lu\""),local_copy);
        command_done = 24;
    }
    else if ( (command_done == 24) ) 
    {
        printf_P(PSTR("}\r\n"));
        command_done = 25;
    }
    else if ( (command_done == 25) ) 
    {
        unsigned long kRuntime= elapsed(&daynight_serial_print_started_at);
        if ((kRuntime) > (serial_print_delay_milsec))
        {
            command_done = 10; /* This keeps looping output forever (until a Rx char anyway) */
        }
    }
}


