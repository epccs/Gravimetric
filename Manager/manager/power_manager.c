/*
Power manager
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
        int battery = analogRead(PWR_V);
        if (battery >= battery_high_limit)
        {
            if (digitalRead(ALT_EN))
            {
                digitalWrite(ALT_EN,LOW);
                enable_alternate_power = 0; // charge is done
            }
            return; // if alt_en is not on do nothing
        }
        int pwm_range = ( (battery_high_limit - battery_low_limit)>>1 ); // half the diff between high and low limit
        unsigned long kRuntime = millis() - alt_pwm_started_at;
        if (battery < (battery_low_limit + pwm_range ) )
        { // half way between high and low limit pwm will occure at 2 sec intervals
            unsigned long offtime = ALT_PWM_PERIOD * ( (battery_high_limit - battery) / pwm_range );
            if (digitalRead(ALT_EN))
            {
                if ( (kRuntime + offtime) > ALT_PWM_PERIOD )
                {
                    digitalWrite(ALT_EN,LOW);
                    alt_pwm_accum_charge_time += kRuntime;
                }
            }
            else 
            {
                if ( kRuntime > ALT_PWM_PERIOD )
                {
                    digitalWrite(ALT_EN,HIGH);
                    if (kRuntime > (ALT_PWM_PERIOD<<1) )
                    {
                        alt_pwm_started_at = millis();
                    }
                    else
                    {
                        alt_pwm_started_at += ALT_PWM_PERIOD;
                    }
                }
            }
            return;
        }
        else if (digitalRead(ALT_EN))
        { // if pwm is not occuring we still need to rest every so often to measure the battery
            if ( (kRuntime + ALT_REST) > ALT_REST_PERIOD )
            {
                digitalWrite(ALT_EN,LOW);
            }
            return;
        }
        else 
        {
            if ( kRuntime > ALT_REST_PERIOD)
            { // end of resting time, start charging
                digitalWrite(ALT_EN,HIGH);
                if (kRuntime > (ALT_REST_PERIOD<<1) )
                {
                    alt_pwm_started_at = millis();
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
        digitalWrite(ALT_EN,LOW);
    }
}