/*
host_shutdown_limits is a library used to load and set values used for host shutdown using EEPROM and I2C. 
Copyright (C) 2020 Ronald Sutherland

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
#include "host_shutdown_limits.h"

HOSTSHUTDOWN_LIM_t shutdown_limit_loaded;
int shutdown_halt_curr_limit;
unsigned long shutdown_ttl_limit;
unsigned long shutdown_delay_limit;
unsigned long shutdown_wearleveling_limit;

uint8_t IsValidShtDwnHaltCurr(int *value) 
{
    if ( ((*value > HOSTSHUTDOWN_LIM_HALTCURR_LIMIT_MIN) && (*value < HOSTSHUTDOWN_LIM_HALTCURR_LIMIT_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t IsValidShtDwnHaltTTL(unsigned long *value) 
{
    if ( ((*value > HOSTSHUTDOWN_LIM_HALT_TTL_MIN) && (*value < HOSTSHUTDOWN_LIM_HALT_TTL_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t IsValidShtDwnDelay(unsigned long *value) 
{
    if ( ((*value > HOSTSHUTDOWN_LIM_DELAY_MIN) && (*value < HOSTSHUTDOWN_LIM_DELAY_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t IsValidShtDwnWearleveling(unsigned long *value) 
{
    if ( ((*value > HOSTSHUTDOWN_LIM_WEARLEVELING_MIN) && (*value < HOSTSHUTDOWN_LIM_WEARLEVELING_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// befor host shutdown is done PWR_I current must be bellow this, save it to EEPROM
uint8_t WriteEEShtDwnHaltCurr() 
{
    if ( eeprom_is_ready() )
    {
        eeprom_update_word( (uint16_t *)(EE_HOSTSHUTDOWN_LIMIT_ADDR+EE_HOSTSHUTDOWN_LIM_HALTCURR_LIMIT), (uint16_t)shutdown_halt_curr_limit);
        return 1;
    }
    else
    {
        return 0;
    }
}

// time to wait for PWR_I to be bellow shutdown_halt_curr_limit and stable for wearleveling, save it to EEPROM
uint8_t WriteEEShtDwnHaltTTL() 
{
    if ( eeprom_is_ready() )
    {
        eeprom_update_dword( (uint32_t *)(EE_HOSTSHUTDOWN_LIMIT_ADDR+EE_HOSTSHUTDOWN_LIM_HALT_TTL), (uint32_t)shutdown_ttl_limit);
        return 1;
    }
    else
    {
        return 0;
    }
}

// time to wait after halt current is valid, but befor checking wearleveling for stable readings, save it to EEPROM
uint8_t WriteEEShtDwnDelay() 
{
    if ( eeprom_is_ready() )
    {
        eeprom_update_dword( (uint32_t *)(EE_HOSTSHUTDOWN_LIMIT_ADDR+EE_HOSTSHUTDOWN_LIM_DELAY), (uint32_t)shutdown_delay_limit);
        return 1;
    }
    else
    {
        return 0;
    }
}

// time PWR_I must be stable befor power down, save it to EEPROM
uint8_t WriteEEShtDwnWearleveling() 
{
    if ( eeprom_is_ready() )
    {
        eeprom_update_dword( (uint32_t *)(EE_HOSTSHUTDOWN_LIMIT_ADDR+EE_HOSTSHUTDOWN_LIM_WEARLEVELING), (uint32_t)shutdown_wearleveling_limit);
        return 1;
    }
    else
    {
        return 0;
    }
}

// load Shutdown Limits from EEPROM (or set defaults)
uint8_t LoadShtDwnLimitsFromEEPROM() 
{
    int tmp_shutdown_halt_curr_limit = eeprom_read_word((uint16_t*)(EE_HOSTSHUTDOWN_LIMIT_ADDR+EE_HOSTSHUTDOWN_LIM_HALTCURR_LIMIT));
    unsigned long temp_shutdown_ttl_limit = eeprom_read_dword((uint32_t*)(EE_HOSTSHUTDOWN_LIMIT_ADDR+EE_HOSTSHUTDOWN_LIM_HALT_TTL));
    unsigned long temp_shutdown_delay_limit = eeprom_read_dword((uint32_t*)(EE_HOSTSHUTDOWN_LIMIT_ADDR+EE_HOSTSHUTDOWN_LIM_DELAY));
    unsigned long temp_shutdown_wearleveling_limit = eeprom_read_dword((uint32_t*)(EE_HOSTSHUTDOWN_LIMIT_ADDR+EE_HOSTSHUTDOWN_LIM_WEARLEVELING));
    uint8_t use_defauts = 0;
    // opps, I did not have "not" (!) in front of each test and was loading uninitialized EEPROM into the values rather than using the defaults. 
    if (!IsValidShtDwnHaltCurr(&tmp_shutdown_halt_curr_limit)) use_defauts = 1; 
    if (!IsValidShtDwnHaltTTL(&temp_shutdown_ttl_limit)) use_defauts = 1;
    if (!IsValidShtDwnDelay(&temp_shutdown_delay_limit)) use_defauts = 1; 
    if (!IsValidShtDwnWearleveling(&temp_shutdown_wearleveling_limit)) use_defauts = 1; 
    if (!use_defauts)
    {
        shutdown_halt_curr_limit = tmp_shutdown_halt_curr_limit; 
        shutdown_ttl_limit = temp_shutdown_ttl_limit;
        shutdown_delay_limit = temp_shutdown_delay_limit;
        shutdown_wearleveling_limit = temp_shutdown_wearleveling_limit;
        shutdown_limit_loaded = HOSTSHUTDOWN_LIM_LOADED;
        return 1;
    }
    else
    {
        // default values are for a Raspberry Pi Zero, 12V8 LA battery, and 5V referance
        shutdown_halt_curr_limit = 30; // a halt R-Pi Z and applicaion has less than 43mA on PWR_I with 12V8: 0.043/((5.0/1024.0)/(0.068*50.0))
        shutdown_ttl_limit = 90000UL; // the R-Pi Z can take some time to halt or startup 60 seconds is default.
        shutdown_delay_limit = 40000UL; // give the SD card 40 seconds to do wearleveling.
        shutdown_wearleveling_limit = 1000UL; // if PWR_I is stable for 1 seconds, accept that to mean wearleveling is done, note this is not proof it is more like hope.
        shutdown_limit_loaded = HOSTSHUTDOWN_LIM_DEFAULT;
        return 0;
    }
}

// save shutdown limits from I2C to EEPROM (if valid)
void ShtDwnLimitsFromI2CtoEE(void)
{
    switch (shutdown_limit_loaded)
    {
    case HOSTSHUTDOWN_LIM_LOADED:
    case HOSTSHUTDOWN_LIM_DEFAULT:
        break;
    case HOSTSHUTDOWN_LIM_HALT_CURR_TOSAVE:
        if ( IsValidShtDwnHaltCurr(&shutdown_halt_curr_limit) )
        {
            if (WriteEEShtDwnHaltCurr())
            {
                shutdown_limit_loaded = HOSTSHUTDOWN_LIM_LOADED;
            }
        }
        break;
    case HOSTSHUTDOWN_LIM_HALT_TTL_TOSAVE:
        if ( IsValidShtDwnHaltTTL(&shutdown_ttl_limit) )
        {
            if (WriteEEShtDwnHaltTTL())
            {
                shutdown_limit_loaded = HOSTSHUTDOWN_LIM_LOADED;
            }
        }
        break;
    case HOSTSHUTDOWN_LIM_DELAY_TOSAVE:
        if ( IsValidShtDwnDelay(&shutdown_delay_limit) )
        {
            if (WriteEEShtDwnDelay())
            {
                shutdown_limit_loaded = HOSTSHUTDOWN_LIM_LOADED;
            }
        }
        break;
    case HOSTSHUTDOWN_LIM_WEARLEVELING_TOSAVE:
        if ( IsValidShtDwnWearleveling(&shutdown_wearleveling_limit) )
        {
            if (WriteEEShtDwnWearleveling())
            {
                shutdown_limit_loaded = HOSTSHUTDOWN_LIM_LOADED;
            }
        }
        break;

    default:
        break;
    }

    // if the value was not valid reload from EEPROM
    if (shutdown_limit_loaded > HOSTSHUTDOWN_LIM_DEFAULT)
    {
        LoadShtDwnLimitsFromEEPROM(); 
    }
}
