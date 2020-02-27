/*
Power manager
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
#include "../lib/io_enum_bsd.h"
#include "battery_limits.h"
#include "power_manager.h"

uint8_t enable_alternate_power;

unsigned long alt_pwm_started_at; // pwm on time
unsigned long alt_pwm_accum_charge_time; // on time accumulation during which pwm was done (e.g., approx LA absorption time)


// enable_alternate_power must be set to start charging
// to do: pwm with a 2 second period, pwm ratio is from battery_high_limit at 25% to battery_low_limit at 75%
void check_if_alt_should_be_on(void)
{
    if (enable_alternate_power)
    {
        int battery = adcAtomic(ADC_CH_PWR_V);
        if (battery >= battery_high_limit)
        {
            if (ioRead(MCU_IO_ALT_EN))
            {
                ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
                enable_alternate_power = 0; // charge is done
            }
            return; // if alt_en is not on do nothing
        }
        int pwm_range = ( (battery_high_limit - battery_low_limit)>>1 ); // half the diff between high and low limit
        unsigned long kRuntime = elapsed(&alt_pwm_started_at);
        if (battery < (battery_low_limit + pwm_range ) )
        { // half way between high and low limit pwm will occure at 2 sec intervals
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
            return;
        }
        else if (ioRead(MCU_IO_ALT_EN))
        { // if pwm is not occuring we still need to rest every so often to measure the battery
            if ( (kRuntime + ALT_REST) > ALT_REST_PERIOD )
            {
                ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
            }
            return;
        }
        else 
        {
            if ( kRuntime > ALT_REST_PERIOD)
            { // end of resting time, start charging
                ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_HIGH);
                if (kRuntime > (ALT_REST_PERIOD<<1) )
                {
                    alt_pwm_started_at = milliseconds();
                }
                else
                {
                    alt_pwm_started_at += ALT_REST_PERIOD;
                }
                return;
            }
        }
    }
    else 
    {
        ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
    }
}