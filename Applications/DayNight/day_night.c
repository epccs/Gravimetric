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

uint8_t daynight_state = 0; 
#define CHK_DAYNIGHT_DELAY 2000UL
static unsigned long chk_daynight_last_started_at;
int daynight_morning_threshold;
int daynight_evening_threshold;
unsigned long daynight_morning_debounce;
unsigned long daynight_evening_debounce;

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
        printf_P(PSTR("{\"day_state\":"));
        command_done = 11;
    }
    else if ( (command_done == 11) )
    { 
        if (daynight_state == DAYNIGHT_START_STATE) 
        {
            printf_P(PSTR("\"STRT\""));
        }

        if (daynight_state == DAYNIGHT_DAY_STATE) 
        {
            printf_P(PSTR("\"DAY\""));
        }

        if (daynight_state == DAYNIGHT_EVENING_DEBOUNCE_STATE) 
        {
            printf_P(PSTR("\"EVE\""));
        }

       if (daynight_state == DAYNIGHT_NIGHTWORK_STATE) 
        {
            printf_P(PSTR("\"NEVT\""));
        }

        if (daynight_state == DAYNIGHT_NIGHT_STATE) 
        {
            printf_P(PSTR("\"NGHT\""));
        }

        if (daynight_state == DAYNIGHT_MORNING_DEBOUNCE_STATE) 
        {
            printf_P(PSTR("\"MORN\""));
        }

        if (daynight_state == DAYNIGHT_DAYWORK_STATE) 
        {
            printf_P(PSTR("\"DEVT\""));
        }

        if (daynight_state == DAYNIGHT_FAIL_STATE) 
        {
            printf_P(PSTR("\"FAIL\""));
        }
        printf_P(PSTR(","));
        command_done = 12;
    }
    else if ( (command_done == 12) ) 
    {
        printf_P(PSTR("\"morning_threshold\":\"%u\","),daynight_morning_threshold);
        command_done = 13;
    }
    else if ( (command_done == 13) ) 
    {
        printf_P(PSTR("\"evening_threshold\":\"%u\","),daynight_evening_threshold);
        command_done = 14;
    }
    else if ( (command_done == 14) ) 
    {
        printf_P(PSTR("\"morning_debounce\":\"%lu\","),daynight_morning_debounce);
        command_done = 15;
    }
    else if ( (command_done == 15) ) 
    {
        printf_P(PSTR("\"evening_debounce\":\"%lu\""),daynight_evening_debounce);
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

/* check for daytime state durring program looping  
    daynight_state: range 0..7
    0 = default at startup, if above Evening Threshold set day, else set night.
    1 = day: wait for evening threshold, set for evening debounce.
    2 = evening_debounce: wait for debounce time, do night_work, however if debouce fails set back to day.
    3 = night_work: do night callback and set night.
    4 = night: wait for morning threshold, set morning_debounce.
    5 = morning_debounce: wait for debounce time, do day_work, however if debouce fails set back to night.
    6 = day_work: do day callback and set for day.
    7 = fail: fail state.

    ALT_V to the manager is used as the light sensor, it has a divider that converts with
    integer_from_adc*((ref_extern_avcc_uV/1.0E6)/1024.0)*(11.0/1.0))
    thus an ALT_V reading of 40 is about 2.1V
    ALT_V reading of 80 is about 4.3V
    ALT_V reading of 160 is about 8.6V
    ALT_V reading of 320 is about 17.18V
*/
uint8_t CheckingDayLight() 
{ 
    unsigned long kRuntime = millis() - chk_daynight_last_started_at;
    if ( kRuntime > CHK_DAYNIGHT_DELAY)
    {
        // check Day-Night state on manager
        uint8_t state = i2c_daynight_state(0); // do not clear work indication bits
        daynight_state = state & 0x0F; // the state is in low nibble

        chk_daynight_last_started_at += CHK_DAYNIGHT_DELAY;

        // high nibble of daynight_state is used by i2C in place of callback functions, the usage is TBD
        // bit 7 is set when night_work needs done
        // bit 6 is set when day_work needs done
        // bit 5 is used with I2C, which if a 1 is passed then bits 7 and 6 are returned with the state
        // bit 4 is used with I2C, which if set with bits 6 and 7 clear on the byte from master then bits 7 and 6 will clear

        // should I run the NightWork callback
        if( (state & (1<<7)) ) 
        { //do the night work callback, e.g. load night light settings at the start of a night
            if (dayState_atNightWork != NULL) dayState_atNightWork();
            i2c_daynight_state(1); // OK to clear work bit
            return 0;
        }

        // should I run the DayWork callback
        if( (state & (1<<6)) )
        { //do the day work callback, e.g. load irrigation settings at the start of a day
            if (dayState_atDayWork != NULL) dayState_atDayWork();
            i2c_daynight_state(1); // OK to clear work bit
            return 0;
        }
    }
    return 0;
}
