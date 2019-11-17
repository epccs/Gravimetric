/*
DayNight state machine is a library that is used to track day and night for manager functions
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

#include <util/delay.h>
#include <avr/io.h>
#include "../lib/timers.h"
#include "../lib/uart.h"
#include "../lib/adc.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"
#include "daynight_limits.h"
#include "daynight_state.h"

// allow some time for ALT_V to have valid data
#define STARTUP_DELAY 11000UL

// change to fail state if is to long
#define DAYNIGHT_TO_LONG 72000000UL

uint8_t daynight_state;

unsigned long dayTmrStarted;

/* check for day-night state durring program looping  
    with low nibble of daynight_state: range 0..7
    0 = default at startup, if above Evening Threshold set day, else set night.
    1 = day: wait for evening threshold, set for evening debounce.
    2 = evening_debounce: wait for debounce time, do night_work, however if debouce fails set back to day.
    3 = night_work: do night callback and set night.
    4 = night: wait for morning threshold, set daynight_morning_debounce.
    5 = daynight_morning_debounce: wait for debounce time, do day_work, however if debouce fails set back to night.
    6 = day_work: do day callback and set for day.
    7 = fail: fail state.

    ALT_V is used as the light sensor, it is converted like this 
    integer_from_adc*((ref_extern_avcc_uV/1.0E6)/1024.0)*(11.0/1.0))
    thus an ALT_V reading of 40 is about 2.1V
    ALT_V reading of 80 is about 4.3V
    ALT_V reading of 160 is about 8.6V
    ALT_V reading of 320 is about 17.18V
    
    high nibble of daynight_state is used by i2C in place of callback functions, the usage is TBD
    bit 7 is set when night_work needs done
    bit 6 is set when day_work needs done
    bit 5 is used with I2C, which if a 1 is passed then bits 7 and 6 are returned with the state
    bit 4 is used with I2C, which if set with the bytes from master/host will clear bits 7 and 6 if they are also clear on the data byte from master/host.
*/
void check_daynight() 
{ 

    // check light on solar pannel with ALT_V, reading are only taken when !ALT_EN.
    int sensor_val = analogRead(ALT_V);
    uint8_t low_nibble_daynight_state = daynight_state & 0x0F; 
    
    if(low_nibble_daynight_state == DAYNIGHT_START_STATE) 
    { 
        unsigned long kRuntime= millis() - dayTmrStarted;
        if ((kRuntime) > ((unsigned long)STARTUP_DELAY)) 
        {
            if(sensor_val > daynight_evening_threshold ) 
            {
                daynight_state = (daynight_state & 0xF0) + DAYNIGHT_DAY_STATE; 
                dayTmrStarted = millis();
            } 
            else 
            {
                daynight_state = (daynight_state & 0xF0) + DAYNIGHT_NIGHT_STATE;
                dayTmrStarted = millis();
            }
        }
        return;
    } 
  
    if(low_nibble_daynight_state == DAYNIGHT_DAY_STATE) 
    { //day
        if (sensor_val < daynight_evening_threshold ) 
        {
            daynight_state = (daynight_state & 0xF0) + DAYNIGHT_EVENING_DEBOUNCE_STATE;
            dayTmrStarted = millis();
        }
        unsigned long kRuntime= millis() - dayTmrStarted;
        if ((kRuntime) > ((unsigned long)DAYNIGHT_TO_LONG)) 
        {
            daynight_state = (daynight_state & 0xF0) + DAYNIGHT_FAIL_STATE;
            dayTmrStarted = millis();
        }
        return;
    }
  
    if(low_nibble_daynight_state == DAYNIGHT_EVENING_DEBOUNCE_STATE) 
    { //evening_debounce
        if (sensor_val < daynight_evening_threshold ) 
        {
            unsigned long kRuntime= millis() - dayTmrStarted;
            if ((kRuntime) > (daynight_evening_debounce)) 
            {
                daynight_state = (daynight_state & 0xF0) + DAYNIGHT_NIGHTWORK_STATE;
                dayTmrStarted = millis();
            } 
        } 
        else 
        {
            daynight_state = (daynight_state & 0xF0) + DAYNIGHT_DAY_STATE;
            dayTmrStarted = millis();
        }
        return;
    }

    if(low_nibble_daynight_state == DAYNIGHT_NIGHTWORK_STATE) 
    { 
        //do the night work callback, e.g. load night light settings at the start of a night
        daynight_state = (daynight_state & 0x7F) + 0x80; //clear bit 7 so I can set it and keep the other bits
        daynight_state = (daynight_state & 0xF0) + DAYNIGHT_NIGHT_STATE;
        return;
    }

    if(low_nibble_daynight_state == DAYNIGHT_NIGHT_STATE) 
    { //night
        if (sensor_val > daynight_morning_threshold ) 
        {
            daynight_state = (daynight_state & 0xF0) + DAYNIGHT_MORNING_DEBOUNCE_STATE;
            dayTmrStarted = millis();
        }
        unsigned long kRuntime= millis() - dayTmrStarted;
        if ((kRuntime) > ((unsigned long)DAYNIGHT_TO_LONG)) 
        {
            daynight_state = (daynight_state & 0xF0) + DAYNIGHT_FAIL_STATE;
            dayTmrStarted = millis();
        }
        return;
    }

    if(low_nibble_daynight_state == DAYNIGHT_MORNING_DEBOUNCE_STATE) 
    { //morning_debounce
        if (sensor_val > daynight_morning_threshold ) 
        {
            unsigned long kRuntime= millis() - dayTmrStarted;
            if ((kRuntime) > (daynight_morning_debounce)) 
            {
                daynight_state = (daynight_state & 0xF0) + DAYNIGHT_DAYWORK_STATE;
            }
        }
        else 
        {
            daynight_state = (daynight_state & 0xF0) + DAYNIGHT_NIGHT_STATE;
        }
        return;
    }

    if(low_nibble_daynight_state == DAYNIGHT_DAYWORK_STATE) 
    { 
        //do the day work callback, e.g. load irrigation settings at the start of a day
        daynight_state = (daynight_state & 0xBF) + 0x40; //clear bit 6 so I can set it and keep the other bits
        daynight_state = (daynight_state & 0xF0) + DAYNIGHT_DAY_STATE;
        return;
    }

    //index out of bounds? 
    if(low_nibble_daynight_state > DAYNIGHT_FAIL_STATE) 
    { 
        daynight_state = (daynight_state & 0xF0) + DAYNIGHT_FAIL_STATE;
        return;
    }
    return;
}