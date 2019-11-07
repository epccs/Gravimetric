/*
battery_limits is a library used to load and set when charging starts and stops using EEPROM and I2C. 
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
#include <avr/eeprom.h> 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../lib/adc.h"
#include "../lib/timers.h"
#include "../lib/pins_board.h"
#include "battery_limits.h"

uint8_t bat_limit_loaded;
int battery_high_limit;
int battery_low_limit;

uint8_t IsValidBatHighLimFor12V(int *value) 
{
    if ( ((*value > BAT12_LIMIT_HIGH_MIN) && (*value < BAT12_LIMIT_HIGH_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t IsValidBatLowLimFor12V(int *value) 
{
    if ( ((*value > BAT12_LIMIT_LOW_MIN) && (*value < BAT12_LIMIT_LOW_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t IsValidBatHighLimFor24V(int *value) 
{
    if ( ((*value > BAT24_LIMIT_HIGH_MIN) && (*value < BAT24_LIMIT_HIGH_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t IsValidBatLowLimFor24V(int *value) 
{
    if ( ((*value > BAT24_LIMIT_LOW_MIN) && (*value < BAT24_LIMIT_LOW_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// wrtite battery high limit (when charging turns off) to EEPROM
uint8_t WriteEEBatHighLim() 
{
    uint16_t tmp_battery_high_limit= eeprom_read_word((uint16_t*)(EE_BAT_LIMIT_ADDR+EE_BAT_LIMIT_OFFSET_HIGH)); 
    if ( eeprom_is_ready() )
    {
        if (tmp_battery_high_limit != battery_high_limit)
        {
            eeprom_write_word( (uint16_t *)(EE_BAT_LIMIT_ADDR+EE_BAT_LIMIT_OFFSET_HIGH), (uint16_t)battery_high_limit);
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

// wrtite battery low limit (when charging turns on) to EEPROM
uint8_t WriteEEBatLowLim() 
{
    uint16_t tmp_battery_low_limit= eeprom_read_word((uint16_t*)(EE_BAT_LIMIT_ADDR+EE_BAT_LIMIT_OFFSET_LOW)); 
    if ( eeprom_is_ready() )
    {
        if (tmp_battery_low_limit != battery_low_limit)
        {
            eeprom_write_word( (uint16_t *)(EE_BAT_LIMIT_ADDR+EE_BAT_LIMIT_OFFSET_LOW), (uint16_t)battery_low_limit);
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

// load Battery Limits from EEPROM (or set defaults)
uint8_t LoadBatLimitsFromEEPROM() 
{
    int tmp_battery_high_limit = eeprom_read_word((uint16_t*)(EE_BAT_LIMIT_ADDR+EE_BAT_LIMIT_OFFSET_HIGH));
    int tmp_battery_low_limit= eeprom_read_word((uint16_t*)(EE_BAT_LIMIT_ADDR+EE_BAT_LIMIT_OFFSET_LOW));
    if ( (IsValidBatHighLimFor12V(&tmp_battery_high_limit) || IsValidBatHighLimFor24V(&tmp_battery_high_limit)) && (IsValidBatLowLimFor12V(&tmp_battery_low_limit) || IsValidBatLowLimFor24V(&tmp_battery_low_limit)) )
    {
        battery_high_limit = (uint16_t)tmp_battery_high_limit; 
        battery_low_limit = (uint16_t)tmp_battery_low_limit; 
        bat_limit_loaded = BAT_LIM_LOADED;
        return 1;
    }
    else
    {
        // default values are for 12V LA measured at PWR_V channel with 5V referance
        battery_high_limit = 397; // 14.2/(((5.0)/1024.0)*(115.8/15.8))
        battery_low_limit = 374; // 13.4/(((5.0)/1024.0)*(115.8/15.8))
        bat_limit_loaded = BAT_LIM_DEFAULT;
        return 0;
    }
}

// save Battery Limits from I2C to EEPROM (if valid)
void BatLimitsFromI2CtoEE(void)
{
    if ( IsValidBatHighLimFor12V(&battery_high_limit) || IsValidBatHighLimFor24V(&battery_high_limit) )
    {
        if (bat_limit_loaded == BAT_HIGH_LIM_TOSAVE)
        {
            if (WriteEEBatHighLim())
            {
                bat_limit_loaded = BAT_LIM_LOADED;
                return; // all done
            }
        }
    }
    else if ( IsValidBatLowLimFor12V(&battery_low_limit) || IsValidBatLowLimFor24V(&battery_low_limit) )
    {
        if (bat_limit_loaded == BAT_LOW_LIM_TOSAVE)
        {
            if (WriteEEBatLowLim())
            {
                bat_limit_loaded = BAT_LIM_LOADED;
                return; // all done
            }
        }
    }
    else
    {
        LoadBatLimitsFromEEPROM(); // ignore values that are not valid
    }
}
