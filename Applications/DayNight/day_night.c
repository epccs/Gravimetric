/*
day-night  is a library that is used to track the day and night for control functions. 
Copyright (C) 2019 Ronald Sutherland

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../lib/parse.h"
#include "../lib/adc.h"
#include "../lib/timers.h"
#include "../lib/rpu_mgr.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"
#include "day_night.h"

uint8_t manager_status;
uint8_t daynight_state = 0; 
#define CHK_DAYNIGHT_DELAY 2000UL

// used to initalize the PointerToWork functions in case they are not used.
void callback_default(void)
{
    return;
}

typedef void (*PointerToWork)(void);
static PointerToWork dayState_atDayWork = callback_default;
static PointerToWork dayState_atNightWork = callback_default;

static unsigned long serial_print_started_at;

/* register (i.e. save a referance to) a function callback that does somthing
 */
void Day_AttachWork( void (*function)(void)  )
{
    dayState_atDayWork = function;
}

void Night_AttachWork( void (*function)(void)  )
{
    dayState_atNightWork = function;
}

void Day(unsigned long serial_print_delay_milsec)
{
    if ( (command_done == 10) )
    {
        serial_print_started_at = millis();
        printf_P(PSTR("{\"state\":\"0x%X\","),daynight_state); // print a hex value
        command_done = 11;
    }
    else if ( (command_done == 11) )
    {
        printf_P(PSTR("\"mgr_status\":\"0x%X\","),manager_status);
        command_done = 12;
    }
    else if ( (command_done == 12) ) 
    {
        int local_copy = i2c_int_access_cmd(MORNING_THRESHOLD,0);
        printf_P(PSTR("\"mor_threshold\":\"%u\","),local_copy);
        command_done = 13;
    }
    else if ( (command_done == 13) ) 
    {
        int local_copy = i2c_int_access_cmd(EVENING_THRESHOLD,0);
        printf_P(PSTR("\"eve_threshold\":\"%u\","),local_copy);
        command_done = 14;
    }
    else if ( (command_done == 14) ) 
    {
        int adc_reads = i2c_get_analogRead_from_manager(ALT_V);
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
        unsigned long kRuntime= millis() - serial_print_started_at;
        if ((kRuntime) > (serial_print_delay_milsec))
        {
            command_done = 10; /* This keeps looping output forever (until a Rx char anyway) */
        }
    }
}

// The daynight state machine running on manager has work bits in high nibble and the state in the low nibble.
void check_daynight_state(void)
{
    uint8_t state = i2c_uint8_access_cmd(DAYNIGHT_STATE,0x20); /*set bit 5 to see work nibble*/

    // test for work bits
    if(state & 0xF0)
    {
        // if bit 7 do night work
        if (state & 0x80)
        {
            dayState_atNightWork();
        }
        // if bit 6 do day work
        if (state & 0x40)
        {
            dayState_atDayWork();
        }
        state = i2c_uint8_access_cmd(DAYNIGHT_STATE,0x10); /*set bit 4 to clear work nibble*/
    }
    daynight_state = state;
}

// Check the manager status byte, 
// interleave with other i2c commands so that a scan loop completes between each call.
void check_manager_status(void)
{
    uint8_t status = i2c_read_status();
    if (twi_errorCode) status = 0x02; // set bit 1 .. to show twi fail
    manager_status = status; // main can use this (e.g., for blink code) 
}
