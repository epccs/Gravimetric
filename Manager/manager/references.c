/*
references is a library used to load and set analog conversion references in EEPROM. 
Copyright (C) 2019 Ronald Sutherland

All rights reserved, specifically, the right to distribute is withheld. Subject to your compliance 
with these terms, you may use this software and any derivatives exclusively with Ronald Sutherland 
products. It is your responsibility to comply with third party license terms applicable to your use 
of third party software (including open source software) that accompany Ronald Sutherland software.

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
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <avr/eeprom.h> 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "../lib/timers.h"
#include "../lib/pins_board.h"
#include "references.h"

volatile uint8_t ref_loaded;
volatile uint8_t ref_select_with_writebit;

// check if reference is a valid float
// 0UL and 0xFFFFFFFFUL are not valid
uint8_t IsValidValForRef() 
{
    uint32_t tmp_cal;
    memcpy(&tmp_cal, &calMap[cal_map].calibration, sizeof tmp_cal);
    if ( (tmp_cal == 0xFFFFFFFFUL) | (tmp_cal == 0x0UL) )
    {
            return 0;
    }
    return 1;
}


uint8_t LoadRefFromEEPROM() 
{
    uint16_t id = eeprom_read_word((uint16_t*)(EE_ANALOG_BASE_ADDR+EE_ANALOG_ID));
    if (id == 0x4144) // 'A' is 0x41 and 'D' is 0x44
    {
        ref_extern_avcc_uV = eeprom_read_dword((uint32_t*)(EE_ANALOG_BASE_ADDR+EE_ANALOG_REF_EXTERN_AVCC));
        if ( IsValidValForAvccRef() ) 
        {
            ref_intern_1v1_uV = eeprom_read_dword((uint32_t*)(EE_ANALOG_BASE_ADDR+EE_ANALOG_REF_INTERN_1V1));
            if ( IsValidValFor1V1Ref() )
            {
                ref_loaded = REF_LOADED;
                return 1;
            }
            else
            { // 1v1 is not used (should it be removed?)
                float tmp_1v1 = 1.08;
                memcpy(&ref_intern_1v1_uV, &tmp_1v1, sizeof ref_intern_1v1_uV);
                ref_loaded = REF_LOADED;
                return 1;
            }
            

        }
    }

    // use defaults
    // on AVR sizeof(float) == sizeof(uint32_t)
    float tmp_avcc = 5.0;
    memcpy(&ref_extern_avcc_uV, &tmp_avcc, sizeof ref_extern_avcc_uV);
    float tmp_1v1 = 1.08;
    memcpy(&ref_intern_1v1_uV, &tmp_1v1, sizeof ref_intern_1v1_uV);
    ref_loaded = REF_DEFAULT;
    return 0;
}

uint8_t WriteRefToEE() 
{
    uint32_t ee_ref_intern_1v1_uV = eeprom_read_dword((uint32_t*)(EE_ANALOG_BASE_ADDR+EE_ANALOG_REF_INTERN_1V1)); 
    if ( eeprom_is_ready() )
    {
        if (ee_ref_intern_1v1_uV != ref_intern_1v1_uV)
        {
            eeprom_update_dword( (uint32_t *)(EE_ANALOG_BASE_ADDR+EE_ANALOG_REF_INTERN_1V1), ref_intern_1v1_uV);
        }
        return 1;
    }
    else
    {
        return 0;
    }
}


// save referances from I2C to EEPROM (if valid)
void ReferanceFromI2CtoEE(void)
{
    if (ref_loaded > REF_DEFAULT)
    {
        if ( IsValidValForAvccRef() && IsValidValFor1V1Ref() )
        {
            uint16_t id = eeprom_read_word((uint16_t*)(EE_ANALOG_BASE_ADDR+EE_ANALOG_ID));
            if (id != 0x4144) // 'A' is 0x41 and 'D' is 0x44
            {
                WriteEeReferenceId();
                return; // that is enough for this loop
            }
            else 
            {
                if (ref_loaded == REF_1V1_TOSAVE)
                {
                    if (WriteEeReference1V1())
                    {
                        ref_loaded = REF_LOADED;
                        return; // all done
                    }
                }
                if (ref_loaded == REF_AVCC_TOSAVE)
                {
                    if (WriteEeReferenceAvcc())
                    {
                        ref_loaded = REF_LOADED; 
                        return; // all done
                    }
                }
            }       
        }
        else
        {
            LoadAnalogRefFromEEPROM(); // ignore values that are not valid
        }
    }
}
