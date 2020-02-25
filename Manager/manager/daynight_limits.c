/*
daynight_limits is a library used to load and set values to operate the day-night state machine using EEPROM and I2C. 
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
#include "../lib/adc.h"
#include "../lib/timers.h"
#include "daynight_limits.h"

uint8_t daynight_values_loaded;
int daynight_morning_threshold;
int daynight_evening_threshold;
unsigned long daynight_morning_debounce;
unsigned long daynight_evening_debounce;

uint8_t IsValidMorningThresholdFor12V(int *value) 
{
    if ( ((*value >= PV12_MORNING_THRESHOLD_MIN) && (*value <= PV12_MORNING_THRESHOLD_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t IsValidEveningThresholdFor12V(int *value) 
{
    if ( ((*value >= PV12_EVENING_THRESHOLD_MIN) && (*value <= PV12_EVENING_THRESHOLD_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t IsValidMorningThresholdFor24V(int *value) 
{
    if ( ((*value >= PV24_MORNING_THRESHOLD_MIN) && (*value <= PV24_MORNING_THRESHOLD_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t IsValidEveningThresholdFor24V(int *value) 
{
    if ( ((*value >= PV24_EVENING_THRESHOLD_MIN) && (*value <= PV24_EVENING_THRESHOLD_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t IsValidMorningDebounce(unsigned long *value) 
{
    if ( ((*value >= MORNING_DEBOUNCE_MIN) && (*value <= MORNING_DEBOUNCE_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t IsValidEveningDebounce(unsigned long *value) 
{
    if ( ((*value >= EVENING_DEBOUNCE_MIN) && (*value <= EVENING_DEBOUNCE_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// wrtite daynight_morning_threshold (when morning debounce starts) to EEPROM
uint8_t WriteEEMorningThreshold() 
{
    uint16_t tmp_daynight_morning_threshold= eeprom_read_word((uint16_t*)(EE_DAYNIGHT_ADDR+EE_DAYNIGHT_MORNING_THRESHOLD_OFFSET)); 
    if ( eeprom_is_ready() )
    {
        if (tmp_daynight_morning_threshold != ((uint16_t)daynight_morning_threshold) )
        {
            eeprom_update_word( (uint16_t *)(EE_DAYNIGHT_ADDR+EE_DAYNIGHT_MORNING_THRESHOLD_OFFSET), (uint16_t)daynight_morning_threshold);
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

// wrtite daynight_evening_threshold (when evening debounce starts) to EEPROM
uint8_t WriteEEEveningThreshold() 
{
    uint16_t tmp_daynight_evening_threshold= eeprom_read_word((uint16_t*)(EE_DAYNIGHT_ADDR+EE_DAYNIGHT_EVENING_THRESHOLD_OFFSET)); 
    if ( eeprom_is_ready() )
    {
        if (tmp_daynight_evening_threshold != ((uint16_t)daynight_evening_threshold) )
        {
            eeprom_update_word( (uint16_t *)(EE_DAYNIGHT_ADDR+EE_DAYNIGHT_EVENING_THRESHOLD_OFFSET), (uint16_t)daynight_evening_threshold);
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

// wrtite daynight_morning_debounce (debounce time in millis) to EEPROM
uint8_t WriteEEMorningDebounce() 
{
    uint32_t tmp_daynight_morning_debounce= eeprom_read_dword((uint32_t*)(EE_DAYNIGHT_ADDR+EE_DAYNIGHT_MORNING_DEBOUNCE_OFFSET)); 
    if ( eeprom_is_ready() )
    {
        if (tmp_daynight_morning_debounce != ((uint32_t)daynight_morning_debounce) )
        {
            eeprom_update_dword( (uint32_t *)(EE_DAYNIGHT_ADDR+EE_DAYNIGHT_MORNING_DEBOUNCE_OFFSET), (uint32_t)daynight_morning_debounce);
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

// wrtite daynight_evening_debounce (debounce time in millis) to EEPROM
uint8_t WriteEEEveningDebounce() 
{
    uint32_t tmp_daynight_evening_debounce= eeprom_read_dword((uint32_t*)(EE_DAYNIGHT_ADDR+EE_DAYNIGHT_EVENING_DEBOUNCE_OFFSET)); 
    if ( eeprom_is_ready() )
    {
        if (tmp_daynight_evening_debounce != ((uint32_t)daynight_evening_debounce) )
        {
            eeprom_update_dword( (uint32_t *)(EE_DAYNIGHT_ADDR+EE_DAYNIGHT_EVENING_DEBOUNCE_OFFSET), (uint32_t)daynight_evening_debounce);
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

// load day-night state machine values from EEPROM (or set defaults)
uint8_t LoadDayNightValuesFromEEPROM() 
{
    uint8_t use_defaults = 0;
    int tmp_daynight_morning_threshold = (int)(eeprom_read_word((uint16_t*)(EE_DAYNIGHT_ADDR+EE_DAYNIGHT_MORNING_THRESHOLD_OFFSET)));
    if ( IsValidMorningThresholdFor12V(&tmp_daynight_morning_threshold) || IsValidMorningThresholdFor24V(&tmp_daynight_morning_threshold) )
    {
        daynight_morning_threshold = tmp_daynight_morning_threshold; 
    }
    else
    {
        use_defaults = 1;
    }
    int tmp_daynight_evening_threshold = (int)(eeprom_read_word((uint16_t*)(EE_DAYNIGHT_ADDR+EE_DAYNIGHT_EVENING_THRESHOLD_OFFSET)));
    if ( (IsValidEveningThresholdFor12V(&tmp_daynight_evening_threshold) || IsValidEveningThresholdFor24V(&tmp_daynight_evening_threshold)) )
    {
        daynight_evening_threshold = tmp_daynight_evening_threshold;
    }
    else
    {
        use_defaults = 1;
    }
    unsigned long tmp_daynight_morning_debounce = (unsigned long)(eeprom_read_dword((uint32_t*)(EE_DAYNIGHT_ADDR+EE_DAYNIGHT_MORNING_DEBOUNCE_OFFSET)));
    if ( IsValidMorningDebounce(&tmp_daynight_morning_debounce) )
    {
        daynight_morning_debounce = tmp_daynight_morning_debounce;
    }
    else
    {
        use_defaults = 1;
    }
    unsigned long tmp_daynight_evening_debounce = (unsigned long)(eeprom_read_dword((uint32_t*)(EE_DAYNIGHT_ADDR+EE_DAYNIGHT_EVENING_DEBOUNCE_OFFSET)));
    if ( IsValidEveningDebounce(&tmp_daynight_evening_debounce) )
    {
        daynight_evening_debounce = tmp_daynight_evening_debounce;
    }
    else
    {
        use_defaults = 1;
    }
    if (use_defaults)
    {
        // default values are for 12V PV measured with ALT_V channel with 5V referance
        daynight_morning_threshold = 80; // 4.3/(((5.0)/1024.0)*(110.0/10.0))
        daynight_evening_threshold = 40; // 2.15/(((5.0)/1024.0)*(110.0/10.0))
        daynight_morning_debounce = 1200000UL; // 20 min
        daynight_evening_debounce = 1200000UL; // 20 min
        daynight_values_loaded = DAYNIGHT_VALUES_DEFAULT;
        return 0;
    }
    else
    {
        daynight_values_loaded = DAYNIGHT_VALUES_LOADED;
        return 1;
    }
}

// Save day-night state machine values from I2C to EEPROM (if valid), one will change per loop, and I2C will take several loop cycles to get another.
void DayNightValuesFromI2CtoEE(void)
{
    if (daynight_values_loaded > DAYNIGHT_VALUES_DEFAULT)
    {
        if (daynight_values_loaded == DAYNIGHT_MORNING_THRESHOLD_TOSAVE)
        {
            if ( IsValidMorningThresholdFor12V(&daynight_morning_threshold) || IsValidMorningThresholdFor24V(&daynight_morning_threshold) )
            {
                if (WriteEEMorningThreshold())
                {
                    daynight_values_loaded = DAYNIGHT_VALUES_LOADED;
                    return; // all done
                }
            }
        }
        if (daynight_values_loaded == DAYNIGHT_EVENING_THRESHOLD_TOSAVE)
        {    
            if ( IsValidEveningThresholdFor12V(&daynight_evening_threshold) || IsValidEveningThresholdFor24V(&daynight_evening_threshold) )
            {
                if (WriteEEEveningThreshold())
                {
                    daynight_values_loaded = DAYNIGHT_VALUES_LOADED;
                    return; // all done
                }
            }
        }
        if (daynight_values_loaded == DAYNIGHT_MORNING_DEBOUNCE_TOSAVE)
        {
            if ( IsValidMorningDebounce(&daynight_morning_debounce) )
            {
                if (WriteEEMorningDebounce())
                {
                    daynight_values_loaded = DAYNIGHT_VALUES_LOADED;
                    return; // all done
                }
            }
        }
        if (daynight_values_loaded == DAYNIGHT_EVENING_DEBOUNCE_TOSAVE)
        {
            if ( IsValidEveningDebounce(&daynight_evening_debounce) )
            {
                if (WriteEEEveningDebounce())
                {
                    daynight_values_loaded = DAYNIGHT_VALUES_LOADED;
                    return; // all done
                }
            }
        }
        LoadDayNightValuesFromEEPROM(); // I guess the values are not valid so reload from EEPROM
    }
}
