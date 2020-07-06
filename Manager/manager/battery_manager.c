/*
Battery manager
Copyright (C) 2019 Ronald Sutherland

All rights reserved, specifically, the right to Redistribut is withheld. Subject 
to your compliance with these terms, you may use this software and derivatives. 

Use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:
1. Source code must retain the above copyright notice, this list of 
conditions and the following disclaimer.
2. Binary derivatives are exclusively for use with Ronald Sutherland 
products.
3. Neither the name of the copyright holders nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS SUPPLIED BY RONALD SUTHERLAND "AS IS". NO WARRANTIES, WHETHER
EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
FOR A PARTICULAR PURPOSE.

IN NO EVENT WILL RONALD SUTHERLAND BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF RONALD SUTHERLAND
HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
THE FULLEST EXTENT ALLOWED BY LAW, RONALD SUTHERLAND'S TOTAL LIABILITY ON ALL
CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO RONALD SUTHERLAND FOR THIS
SOFTWARE.
*/

#include <stdbool.h>
#include <util/delay.h>
#include <avr/io.h>
#include "../lib/timers_bsd.h"
#include "../lib/uart0_bsd.h"
#include "../lib/adc_bsd.h"
#include "../lib/twi0_bsd.h"
#include "../lib/io_enum_bsd.h"
#include "i2c_callback.h"
#include "daynight_state.h"
#include "host_shutdown_manager.h"
#include "battery_limits.h"
#include "battery_manager.h"

BATTERYMGR_STATE_t bm_state;

unsigned long alt_pwm_started_at; // pwm on time
unsigned long alt_pwm_accum_charge_time; // on time accumulation during which pwm was done (e.g., approx LA absorption time)

uint8_t bm_callback_address;
uint8_t bm_callback_route;
uint8_t bm_callback_poke;
uint8_t bm_enable;
TWI0_LOOP_STATE_t loop_state;
unsigned long ontime;
unsigned long next_ontime;

uint8_t fail_wip;

// battery manager: controls ALT_EN pin PB3 to charge a battery connected on the power input
// bm_enable must be set to start charging
// to do: pwm with a 2 second period, pwm ratio is from battery_high_limit at 25% to battery_low_limit at 75%
void check_battery_manager(void)
{
    // TWI waiting to finish (ignore daynight changes while TWI is waiting)
    if (i2c_callback_waiting(&loop_state)) return; 
    
    // if somthing else is using twi then my loop_state should allow getting to this, but I want to skip this state machine
    if (i2c_is_in_use) return;

    // if battery limits are changing skip this state machine
    if (bat_limit_loaded > BAT_LIM_DEFAULT) return;

    // if not day then reset bm_state to START
    if (daynight_state != DAYNIGHT_STATE_DAY)
    {
        if ((bm_state > BATTERYMGR_STATE_START) && (bm_state < BATTERYMGR_STATE_FAIL)) // exclude START and FAIL
        {
            ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
            bm_state = BATTERYMGR_STATE_START;
            if (bm_callback_address && bm_callback_route)
            {
                if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                i2c_callback(bm_callback_address, bm_callback_route, bm_state, &loop_state); // update application
            }
        }
        return;
    }

    // poke, is used to update the application after it has been reset and restart bm if it has failed
    if (bm_callback_poke)
    {
        if (bm_callback_address && bm_callback_route)
        {
            if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
            i2c_callback(bm_callback_address, bm_callback_route, bm_state, &loop_state); // update application
        }
        bm_callback_poke = 0;
        if (bm_state == BATTERYMGR_STATE_FAIL) bm_state = BATTERYMGR_STATE_START; // restart if bm had failed
        return;
    }

    if (bm_enable) // also daynight_state == DAY
    {
        int battery = adcAtomic(ADC_CH_PWR_V);
        unsigned long kRuntime = elapsed(&alt_pwm_started_at);
        switch (bm_state)
        {
        case BATTERYMGR_STATE_START:
            ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW); // start a pwm period, but with out alternat enabled
            alt_pwm_started_at = milliseconds();
            ontime = 0;
            next_ontime = 0;
            bm_state = BATTERYMGR_STATE_PWM_MODE_OFF;
            if (bm_callback_address && bm_callback_route)
            {
                if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                i2c_callback(bm_callback_address, bm_callback_route, bm_state, &loop_state); // update application
            }
            break;

        case BATTERYMGR_STATE_CC_REST:
            if (kRuntime > ALT_REST)
            {
                // rest is over
                ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_HIGH);
                bm_state = BATTERYMGR_STATE_CC_MODE;
                if (bm_callback_address && bm_callback_route)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(bm_callback_address, bm_callback_route, bm_state, &loop_state); // update application
                }
            }
            break;

        case BATTERYMGR_STATE_CC_MODE:
            // if the battery goes to low and host is UP start the software shutdown process
            if (battery < battery_host_limit)
            {
                if (shutdown_state == HOSTSHUTDOWN_STATE_UP)
                {
                    shutdown_state = HOSTSHUTDOWN_STATE_SW_HALT;
                }
                break;
            }

            // if the battery goes even lower perhaps the application can be held in reset and the manager could sleep.

            // when battery is above low limit pwm operates with 2 sec intervals
            if (battery > battery_low_limit)
            { 
                ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_HIGH); // start a pwm period
                alt_pwm_started_at = milliseconds();
                ontime = (ALT_PWM_PERIOD - 200); // with max ontime duty
                next_ontime = (ALT_PWM_PERIOD - 200); // replaced befor changing bm_state to PWM_MODE_ON but max duty is more correct than zero
                bm_state = BATTERYMGR_STATE_PWM_MODE_OFF;
                if (bm_callback_address && bm_callback_route)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(bm_callback_address, bm_callback_route, bm_state, &loop_state); // update application
                }
                break;
            }

            if (kRuntime > ALT_REST_PERIOD)
            { 
                // next period, starts with a rest
                ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
                bm_state = BATTERYMGR_STATE_CC_REST;
                alt_pwm_started_at += ALT_REST_PERIOD;
                if (bm_callback_address && bm_callback_route)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(bm_callback_address, bm_callback_route, bm_state, &loop_state); // update application
                }
            }
            break;

        case BATTERYMGR_STATE_PWM_MODE_OFF:

            // check if on control is enabled and end the on_time if it is enabled
            if (ioRead(MCU_IO_ALT_EN))
            {
                ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
            }

            // when battery is at or above the high limit it is done
            if (battery >= battery_high_limit)
            {
                bm_state = BATTERYMGR_STATE_DONE; // charge is done
                if (bm_callback_address && bm_callback_route)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(bm_callback_address, bm_callback_route, bm_state, &loop_state); // update application
                }
                break;
            }

            if (ontime)
            {
                if (kRuntime > (ALT_PWM_PERIOD - ontime)) // > offtime
                {
                    bm_state = BATTERYMGR_STATE_PWM_MODE_ON;

                    // this is best place to measure the battery value for next PWM cycle
                    next_ontime =  ( (ALT_PWM_PERIOD*battery_high_limit - ALT_PWM_PERIOD*battery) / (battery_high_limit - battery_low_limit) );

                    if (bm_callback_address && bm_callback_route)
                    {
                        if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                        i2c_callback(bm_callback_address, bm_callback_route, bm_state, &loop_state); // update application
                    }
                }
            }
            else
            {
                if ( (next_ontime >= 200) && (next_ontime <= (ALT_PWM_PERIOD - 200)) )
                {
                    ontime = next_ontime;
                }
                else
                {
                    if (next_ontime < 200) ontime = 200; // to small
                    if (next_ontime > (ALT_PWM_PERIOD - 200)) ontime = (ALT_PWM_PERIOD - 200); // to big
                }
                ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_HIGH);
            }
            break;


        case BATTERYMGR_STATE_PWM_MODE_ON:
            // check if on control is disabled and start the on_time if it is disabled
            if (!ioRead(MCU_IO_ALT_EN))
            {
                ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_HIGH);
            }

            // new pwm period
            if (kRuntime > ALT_PWM_PERIOD)
            {
                if (battery > battery_low_limit)
                {
                    alt_pwm_accum_charge_time += ontime; // ccumulate the ontime
                    alt_pwm_started_at += ALT_PWM_PERIOD;
                    ontime = 0; // next_ontime will be moved to ontime during PWM_MODE_OFF
                    bm_state = BATTERYMGR_STATE_PWM_MODE_OFF;
                    if (bm_callback_address && bm_callback_route)
                    {
                        if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                        i2c_callback(bm_callback_address, bm_callback_route, bm_state, &loop_state); // update application
                    }
                }
                else
                {
                    bm_state = BATTERYMGR_STATE_CC_REST;
                    if (bm_callback_address && bm_callback_route)
                    {
                        if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                        i2c_callback(bm_callback_address, bm_callback_route, bm_state, &loop_state); // update application
                    }
                }
            }
            break;

        case BATTERYMGR_STATE_DONE:
            // only if the battery is less than the low limit start it again
            if (battery <= battery_low_limit )
            {
                ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
                alt_pwm_started_at = milliseconds();
                bm_state = BATTERYMGR_STATE_CC_REST;
                if (bm_callback_address && bm_callback_route)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(bm_callback_address, bm_callback_route, bm_state, &loop_state); // update application
                }
            }
            break;

        case BATTERYMGR_STATE_FAIL:
            // if somthing sets fail state then report it and shutdown charge control, 
            switch (fail_wip) // failure work in progress, it takes a few loops to finish since the TWI reporting is non-blocking 
            {
            case 0: // turn off alternat power input and report failure
                ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
                if (bm_callback_address && bm_callback_route)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(bm_callback_address, bm_callback_route, bm_state, &loop_state); // update application
                }
                fail_wip = 1;
                break;

                case 1: // turn off the alternat power control state machine
                    // bm_callback_route = 0;
                    // bm_callback_address =0;
                    bm_enable = 0;
                    fail_wip = 0;
                    break;

                default:
                    break;
            }
            break;
        
        default:
            break;
        }
    }
    else // not bm_enable
    {
        if ((bm_state > BATTERYMGR_STATE_START) && (bm_state < BATTERYMGR_STATE_FAIL)) // exclude START and FAIL
        {
            ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
            bm_state = BATTERYMGR_STATE_START;
            if (bm_callback_address && bm_callback_route)
            {
                if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                i2c_callback(bm_callback_address, bm_callback_route, bm_state, &loop_state); // update application
            }
        }
        return;
    }
}
