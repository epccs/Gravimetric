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
#include "power_manager.h"

uint8_t enable_alternate_power;
uint8_t enable_sbc_power;

uint16_t alt_count; // count the number of times the battery is at the charge limit
int battery_high_limit;
int battery_low_limit;

// enable_alternate_power must be set to start charging
// to do: pwm with a 2 second period, 
//            pwm ratio is from battery_high_limit at 25% to battery_low_limit at 75%
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
                alt_count += 1; // count the number of times the battery is at the charge limit
            }
        }
        else if (battery < battery_low_limit)
        {
            digitalWrite(ALT_EN,HIGH);
            // alt_pwm_started_at = millis();
        }
    }
    else 
    {
        digitalWrite(ALT_EN,LOW);
    }
}