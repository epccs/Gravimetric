/*
battery_limits is a library used to load and set when charging starts and stops using EEPROM and I2C. 
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <avr/eeprom.h>
#include "../lib/adc_bsd.h"
#include "../lib/io_enum_bsd.h"
#include "battery_limits.h"

BAT_LIM_t bat_limit_loaded;
int battery_high_limit;
int battery_low_limit;
int battery_host_limit;

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

uint8_t IsValidBatHostLimFor12V(int *value) 
{
    if ( ((*value > BAT12_LIMIT_HOST_MIN) && (*value < BAT12_LIMIT_HOST_MAX)) )
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

uint8_t IsValidBatHostLimFor24V(int *value) 
{
    if ( ((*value > BAT24_LIMIT_HOST_MIN) && (*value < BAT24_LIMIT_HOST_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// wrtite battery high limit (when charging and PWM turns off) to EEPROM
uint8_t WriteEEBatHighLim() 
{
    if ( eeprom_is_ready() )
    {
        eeprom_update_word( (uint16_t *)(EE_BAT_LIMIT_ADDR+EE_BAT_LIMIT_OFFSET_HIGH), (uint16_t)battery_high_limit);
        return 1;
    }
    else
    {
        return 0;
    }
}

// wrtite battery low limit (when PWM turns on) to EEPROM
uint8_t WriteEEBatLowLim() 
{
    if ( eeprom_is_ready() )
    {
        eeprom_update_word( (uint16_t *)(EE_BAT_LIMIT_ADDR+EE_BAT_LIMIT_OFFSET_LOW), (uint16_t)battery_low_limit);
        return 1;
    }
    else
    {
        return 0;
    }
}

// wrtite battery host limit (when host turns off) to EEPROM
uint8_t WriteEEBatHostLim() 
{
    if ( eeprom_is_ready() )
    {
        eeprom_update_word( (uint16_t *)(EE_BAT_LIMIT_ADDR+EE_BAT_LIMIT_OFFSET_HOST), (uint16_t)battery_host_limit);
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
    int tmp_battery_low_limit = eeprom_read_word((uint16_t*)(EE_BAT_LIMIT_ADDR+EE_BAT_LIMIT_OFFSET_LOW));
    int tmp_battery_host_limit = eeprom_read_word((uint16_t*)(EE_BAT_LIMIT_ADDR+EE_BAT_LIMIT_OFFSET_HOST));
    uint8_t use_defauts = 0;
    if ( !(IsValidBatHighLimFor12V(&tmp_battery_high_limit) || IsValidBatHighLimFor24V(&tmp_battery_high_limit)) ) use_defauts = 1;
    if ( !(IsValidBatLowLimFor12V(&tmp_battery_low_limit) || IsValidBatLowLimFor24V(&tmp_battery_low_limit)) ) use_defauts = 1;
    if ( !(IsValidBatHostLimFor12V(&tmp_battery_host_limit) || IsValidBatHostLimFor24V(&tmp_battery_host_limit)) ) use_defauts = 1;
    if (!use_defauts)
    {
        battery_high_limit = (uint16_t)tmp_battery_high_limit; 
        battery_low_limit = (uint16_t)tmp_battery_low_limit;
        battery_host_limit = (uint16_t)tmp_battery_host_limit; 
        bat_limit_loaded = BAT_LIM_LOADED;
        return 1;
    }
    else
    {
        int battery_value = adc[MCU_IO_PWR_V];
        if ( ((battery_value > BAT24_LIMIT_LOW_MIN) && (battery_value < BAT24_LIMIT_HIGH_MAX)) ) 
        {
            // default values are for 12V LA measured at PWR_V channel with 5V referance
            battery_high_limit = 794; // 28.4/(((5.0)/1024.0)*(115.8/15.8))
            battery_low_limit = 469; // 16.8/(((5.0)/1024.0)*(115.8/15.8))
            battery_host_limit = 615; // 22.0/(((5.0)/1024.0)*(115.8/15.8))
            bat_limit_loaded = BAT_LIM_DEFAULT;
        }
        else if ( ((battery_value > BAT12_LIMIT_LOW_MIN) && (battery_value < BAT12_LIMIT_HIGH_MAX)) )
        {
            // default values are for 12V LA measured at PWR_V channel with 5V referance
            battery_high_limit = 397; // 14.2/(((5.0)/1024.0)*(115.8/15.8))
            battery_low_limit = 374; // 13.4/(((5.0)/1024.0)*(115.8/15.8))
            battery_host_limit = 307; // 11.0/(((5.0)/1024.0)*(115.8/15.8))
            bat_limit_loaded = BAT_LIM_DEFAULT;
        }
        else
        {
            bat_limit_loaded = BAT_LIM_DELAY_LOAD; // also blocks the bm state machine from running
        }
        return 0;
    }
}

// save Battery Limits from I2C to EEPROM (if valid)
void BatLimitsFromI2CtoEE(void)
{
    if (bat_limit_loaded > BAT_LIM_DEFAULT)
    {
        if (bat_limit_loaded == BAT_LIM_HIGH_TOSAVE)
        {
            if ( IsValidBatHighLimFor12V(&battery_high_limit) || IsValidBatHighLimFor24V(&battery_high_limit) )
            {
                if (WriteEEBatHighLim())
                {
                    bat_limit_loaded = BAT_LIM_LOADED;
                    return; // all done
                }
            }
        }
        if (bat_limit_loaded == BAT_LIM_LOW_TOSAVE)
        {    
            if ( IsValidBatLowLimFor12V(&battery_low_limit) || IsValidBatLowLimFor24V(&battery_low_limit) )
            {
                if (WriteEEBatLowLim())
                {
                    bat_limit_loaded = BAT_LIM_LOADED;
                    return; // all done
                }
            }
        }
        if (bat_limit_loaded == BAT_LIM_HOST_TOSAVE)
        {    
            if ( IsValidBatHostLimFor12V(&battery_host_limit) || IsValidBatHostLimFor24V(&battery_host_limit) )
            {
                if (WriteEEBatHostLim())
                {
                    bat_limit_loaded = BAT_LIM_LOADED;
                    return; // all done
                }
            }
        }
        LoadBatLimitsFromEEPROM(); // I guess the values are not valid so reload from EEPROM
    }
}
