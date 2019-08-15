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
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"
#include "day_night.h"

// analog reading of 5*5.0/1024.0 is about 0.024V
// analog reading of 10*5.0/1024.0 is about 0.049V
#define MORNING_THRESHOLD 10
#define STARTUP_DELAY 1000UL
#define EVENING_THRESHOLD 5
// 900,000 millis is 15 min
#define EVENING_DEBOUCE 900000UL
#define MORNING_DEBOUCE 900000UL
#define DAYNIGHT_TO_LONG 72000000UL
// 72 Meg millis() is 20hr, which is past the longest day or night so somthing has went wrong.
/* note the UL is a way to tell the compiler that a numerical literal is of type unsigned long */

#if MORNING_THRESHOLD > 0x3FF
#   error ADC maxim size is 0x3FF 
#endif

#if EVENING_THRESHOLD > 0x3FF
#   error ADC maxim size is 0x3FF 
#endif

#if MORNING_THRESHOLD - EVENING_THRESHOLD < 0x04
#   error ADC hysteresis of 4 should be allowed
#endif

// public
int morning_threshold = MORNING_THRESHOLD;
int evening_threshold = EVENING_THRESHOLD;
unsigned long evening_debouce = (unsigned long)EVENING_DEBOUCE;
unsigned long morning_debouce = (unsigned long)MORNING_DEBOUCE;

// local
static uint8_t alt_power_is_checking_light =0;
static uint8_t alt_power_value = 0;
static uint8_t dayState = 0; 
static unsigned long dayTmrStarted;
#define CHK_SOLAR_DELAY 2000UL
static unsigned long chk_light_started_at;

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
        uint8_t state = DayState();

        if (state == DAYNIGHT_START_STATE) 
        {
            printf_P(PSTR("\"STRT\""));
        }

        if (state == DAYNIGHT_DAY_STATE) 
        {
            printf_P(PSTR("\"DAY\""));
        }

        if (state == DAYNIGHT_EVENING_DEBOUNCE_STATE) 
        {
            printf_P(PSTR("\"EVE\""));
        }

       if (state == DAYNIGHT_NIGHTWORK_STATE) 
        {
            printf_P(PSTR("\"NEVT\""));
        }

        if (state == DAYNIGHT_NIGHT_STATE) 
        {
            printf_P(PSTR("\"NGHT\""));
        }

        if (state == DAYNIGHT_MORNING_DEBOUNCE_STATE) 
        {
            printf_P(PSTR("\"MORN\""));
        }

        if (state == DAYNIGHT_DAYWORK_STATE) 
        {
            printf_P(PSTR("\"DEVT\""));
        }

        if (state == DAYNIGHT_FAIL_STATE) 
        {
            printf_P(PSTR("\"FAIL\""));
        }
        printf_P(PSTR("}\r\n"));
        command_done = 12;
    }
    else if ( (command_done == 12) ) 
    { // delay between JSON printing
        unsigned long kRuntime= millis() - serial_print_started_at;
        if ((kRuntime) > (serial_print_delay_milsec))
        {
            command_done = 10; /* This keeps looping output forever (until a Rx char anyway) */
        }
    }
}

/* check for daytime state durring program looping  
    dayState: range 0..7
    0 = default at startup, if above Evening Threshold set day, else set night.
    1 = day: wait for evening threshold, set for evening debounce.
    2 = evening_debounce: wait for debounce time, do night_work, however if debouce fails set back to day.
    3 = night_work: do night callback and set night.
    4 = night: wait for morning threshold, set morning_debounce.
    5 = morning_debounce: wait for debounce time, do day_work, however if debouce fails set back to night.
    6 = day_work: do day callback and set for day.
    7 = fail: fail state.

    ALT_V (ADC4) is used as the light sensor, it has a divider that converts with
    integer_from_adc*((ref_extern_avcc_uV/1.0E6)/1024.0)*(11.0/1.0))
    thus an ALT_V reading of 40 is about 2.1V
    ALT_V reading of 80 is about 4.3V
    ALT_V reading of 160 is about 8.6V
    ALT_V reading of 320 is about 17.18V
*/
uint8_t CheckingDayLight() 
{ 
    unsigned long kRuntime = millis() - chk_light_started_at;
    if ( kRuntime > CHK_SOLAR_DELAY)
    {
        // check light from solar pannel with ALT power disabled
        if (!alt_power_is_checking_light)
        {
            alt_power_is_checking_light = 1;
            alt_power_value = digitalRead(ALT_EN);
            digitalWrite(ALT_EN,LOW);
        }
        
        // delay for 500mSec to let ADC input filter cap charge
        if ( kRuntime <= CHK_SOLAR_DELAY+500UL) return 1;
        
        // next check
        chk_light_started_at += CHK_SOLAR_DELAY; 

        // take sensor value and return power to its previous state
        int sensor_val = analogRead(ALT_V);
        digitalWrite(ALT_EN,alt_power_value);
        alt_power_is_checking_light = 0;
        
        if(dayState == DAYNIGHT_START_STATE) 
        { 
            unsigned long kRuntime= millis() - dayTmrStarted;
            if ((kRuntime) > ((unsigned long)STARTUP_DELAY)) 
            {
                if(sensor_val > evening_threshold ) 
                {
                    dayState = DAYNIGHT_DAY_STATE; 
                    dayTmrStarted = millis();
                } 
                else 
                {
                    dayState = DAYNIGHT_NIGHT_STATE;
                    dayTmrStarted = millis();
                }
            }
            return 0;
        } 
      
        if(dayState == DAYNIGHT_DAY_STATE) 
        { //day
            if (sensor_val < evening_threshold ) 
            {
                dayState = DAYNIGHT_EVENING_DEBOUNCE_STATE;
                dayTmrStarted = millis();
            }
            unsigned long kRuntime= millis() - dayTmrStarted;
            if ((kRuntime) > ((unsigned long)DAYNIGHT_TO_LONG)) 
            {
                dayState = DAYNIGHT_FAIL_STATE;
                dayTmrStarted = millis();
            }
            return 0;
        }
      
        if(dayState == DAYNIGHT_EVENING_DEBOUNCE_STATE) 
        { //evening_debounce
            if (sensor_val < evening_threshold ) 
            {
                unsigned long kRuntime= millis() - dayTmrStarted;
                if ((kRuntime) > (evening_debouce)) 
                {
                    dayState = DAYNIGHT_NIGHTWORK_STATE;
                    dayTmrStarted = millis();
                } 
            } 
            else 
            {
                dayState = DAYNIGHT_DAY_STATE;
                dayTmrStarted = millis();
            }
            return 0;
        }

        if(dayState == DAYNIGHT_NIGHTWORK_STATE) 
        { //do the night work callback, e.g. load night light settings at the start of a night
            if (dayState_atNightWork != NULL) dayState_atNightWork();
            dayState = DAYNIGHT_NIGHT_STATE;
            return 0;
        }

        if(dayState == DAYNIGHT_NIGHT_STATE) 
        { //night
            if (sensor_val > morning_threshold ) 
            {
                dayState = DAYNIGHT_MORNING_DEBOUNCE_STATE;
                dayTmrStarted = millis();
            }
            unsigned long kRuntime= millis() - dayTmrStarted;
            if ((kRuntime) > ((unsigned long)DAYNIGHT_TO_LONG)) 
            {
                dayState = DAYNIGHT_FAIL_STATE;
                dayTmrStarted = millis();
            }
            return 0;
        }

        if(dayState == DAYNIGHT_MORNING_DEBOUNCE_STATE) 
        { //morning_debounce
            if (sensor_val > morning_threshold ) 
            {
                unsigned long kRuntime= millis() - dayTmrStarted;
                if ((kRuntime) > (morning_debouce)) 
                {
                    dayState = DAYNIGHT_DAYWORK_STATE;
                }
            }
            else 
            {
                dayState = DAYNIGHT_NIGHT_STATE;
            }
            return 0;
        }

        if(dayState == DAYNIGHT_DAYWORK_STATE) 
        { //do the day work callback, e.g. load irrigation settings at the start of a day
            if (dayState_atDayWork != NULL) dayState_atDayWork();
            dayState = DAYNIGHT_DAY_STATE;
            return 0;
        }

        //index out of bounds? 
        if(dayState > DAYNIGHT_FAIL_STATE) 
        { 
            dayState = DAYNIGHT_FAIL_STATE;
            return 0;
        }
    }
    return 0;
}

uint8_t DayState(void) 
{
    return dayState;
}
