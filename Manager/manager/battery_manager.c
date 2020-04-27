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
#include "battery_limits.h"
#include "battery_manager.h"

BATTERYMGR_STATE_t batmgr_state;

unsigned long alt_pwm_started_at; // pwm on time
unsigned long alt_pwm_accum_charge_time; // on time accumulation during which pwm was done (e.g., approx LA absorption time)

uint8_t enable_alternate_callback_address;
uint8_t battery_state_callback_cmd;
TWI0_LOOP_STATE_t loop_state;

uint8_t fail_wip;

// enable_alternate_callback_address must be set to start charging
// to do: pwm with a 2 second period, pwm ratio is from battery_high_limit at 25% to battery_low_limit at 75%
void check_if_alt_should_be_on(void)
{
    // TWI waiting to finish (ignore daynight changes while TWI is waiting)
    if (i2c_callback_waiting(&loop_state)) return; 
    
    // if somthing else is using twi then my loop_state should allow getting to this, but I want to skip this state machine
    if (i2c_is_in_use) return;

    // if limits are changing skip this state machine
    if (bat_limit_loaded >= BAT_LIM_DEFAULT) return;

    if ( (enable_alternate_callback_address) ) // disable with callback set to zero (note the i2c callback also needs battery_state_callback_cmd)
    {
        int battery = adcAtomic(ADC_CH_PWR_V);
        if (batmgr_state < BATTERYMGR_STATE_DONE)
        {
            // when battery is at or above the high limit it is done
            if (battery >= battery_high_limit)
            {
                ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
                batmgr_state = BATTERYMGR_STATE_DONE; // charge is done
                if (battery_state_callback_cmd)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(enable_alternate_callback_address, battery_state_callback_cmd, batmgr_state, &loop_state); // update application
                }
                return; // if alt_en is not on do nothing
            }

            // when battery is half way between high and low limit pwm will start at 2 sec intervals
            int pwm_range = ( (battery_high_limit - battery_low_limit)>>1 ); // half the diff between high and low limit
            unsigned long kRuntime = elapsed(&alt_pwm_started_at);
            if (battery > (battery_low_limit + pwm_range ) )
            { 
                unsigned long offtime = ALT_PWM_PERIOD * ( (battery_high_limit - battery) / pwm_range );
                if (ioRead(MCU_IO_ALT_EN))
                {
                    if ( (kRuntime + offtime) > ALT_PWM_PERIOD )
                    {
                        ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
                        alt_pwm_accum_charge_time += kRuntime;
                    }
                }
                else 
                {
                    if ( kRuntime > ALT_PWM_PERIOD )
                    {
                        ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_HIGH);
                        if (kRuntime > (ALT_PWM_PERIOD<<1) )
                        {
                            alt_pwm_started_at = milliseconds();
                        }
                        else
                        {
                            alt_pwm_started_at += ALT_PWM_PERIOD;
                        }
                    }
                }
                batmgr_state = BATTERYMGR_STATE_PWM_MODE;
                if (battery_state_callback_cmd)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(enable_alternate_callback_address, battery_state_callback_cmd, batmgr_state, &loop_state); // update application
                }
                return;
            }

            // CC mode, but we still need to rest every so often to measure the battery
            if (battery < (battery_low_limit + pwm_range ))
            {
                if ( (kRuntime > ALT_REST) && (batmgr_state == BATTERYMGR_STATE_CC_REST))
                {
                    // rest is over
                    ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_HIGH);
                    batmgr_state = BATTERYMGR_STATE_CC_MODE;
                    if (battery_state_callback_cmd)
                    {
                        if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                        i2c_callback(enable_alternate_callback_address, battery_state_callback_cmd, batmgr_state, &loop_state); // update application
                    }
                    return;
                }
                if (batmgr_state == BATTERYMGR_STATE_CC_MODE)
                { 
                    if (kRuntime > ALT_REST_PERIOD )
                    {
                        // next period, starts with a rest
                        ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
                        batmgr_state = BATTERYMGR_STATE_CC_REST;
                        alt_pwm_started_at += ALT_REST_PERIOD;
                        if (battery_state_callback_cmd)
                        {
                            if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                            i2c_callback(enable_alternate_callback_address, battery_state_callback_cmd, batmgr_state, &loop_state); // update application
                        }
                        return;
                    }
                }

                // battery is in CC mode range, but state was not set so start with rest
                ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
                alt_pwm_started_at = milliseconds();
                batmgr_state = BATTERYMGR_STATE_CC_REST;
                if (battery_state_callback_cmd)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(enable_alternate_callback_address, battery_state_callback_cmd, batmgr_state, &loop_state); // update application
                }
                return;
            }
        }
        else
        {
            // when the battery is bellow the low limit it is time to charge
            if ((batmgr_state == BATTERYMGR_STATE_DONE) && (battery <= battery_low_limit))
            {
                ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
                alt_pwm_started_at = milliseconds();
                batmgr_state = BATTERYMGR_STATE_CC_REST;
                if (battery_state_callback_cmd)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(enable_alternate_callback_address, battery_state_callback_cmd, batmgr_state, &loop_state); // update application
                }
                return;
            }

            // if somthing sets fail state then report it and shutdown charge control, 
            if (batmgr_state == BATTERYMGR_STATE_FAIL)
            {
                switch (fail_wip) // failure work in progress, it takes a few loops to finish since the TWI reporting is non-blocking 
                {
                case 0: // turn off alternat power input and report failure
                    ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
                    if (battery_state_callback_cmd)
                    {
                        if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                        i2c_callback(enable_alternate_callback_address, battery_state_callback_cmd, batmgr_state, &loop_state); // update application
                    }
                    fail_wip = 1;
                    break;

                case 1: // turn off the alternat power control state machine
                    battery_state_callback_cmd = 0;
                    enable_alternate_callback_address =0;
                    fail_wip = 0;
                    break;

                default:
                    break;
                }
                return;
            }
            return;
        }
        
    }
    else // not enabled
    {
        if (batmgr_state < BATTERYMGR_STATE_DONE)
        {
            ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
            batmgr_state = BATTERYMGR_STATE_DONE;
            // callback not enabled
            return;
        }
        else
        {
            // done or fail, so do nothing
            return;
        }
        
    }
}