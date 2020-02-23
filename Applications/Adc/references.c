/*
references is a library used to load and set analog conversion references in EEPROM. 
Copyright (C) 2019 Ronald Sutherland

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES 
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE 
FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY 
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, 
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, 
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Note the library files are LGPL, e.g., you need to publish changes of them but can derive from this 
source and copyright or distribute as you see fit (it is Zero Clause BSD).

https://en.wikipedia.org/wiki/BSD_licenses#0-clause_license_(%22Zero_Clause_BSD%22)
*/
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <avr/eeprom.h> 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../lib/parse.h"
#include "../lib/timers.h"
#include "references.h"

//The EEPROM memory usage is as follows. 
#define EE_ANALOG_BASE_ADDR 30
// each setting is at this byte offset
#define EE_ANALOG_ID 0
#define EE_ANALOG_REF_EXTERN_AVCC 2
#define EE_ANALOG_REF_INTERN_1V1 6

#define REF_EXTERN_AVCC_MAX 5500000UL
#define REF_EXTERN_AVCC_MIN 4500000UL
#define REF_INTERN_1V1_MAX 1300000UL
#define REF_INTERN_1V1_MIN 900000UL

uint8_t ref_loaded;
uint32_t ref_extern_avcc_uV;
uint32_t ref_intern_1v1_uV;

uint8_t IsValidValForAvccRef(uint32_t *value) 
{
    if ( ((*value > REF_EXTERN_AVCC_MIN) && (*value < REF_EXTERN_AVCC_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t IsValidValFor1V1Ref(uint32_t *value) 
{
    if ( ((*value > REF_INTERN_1V1_MIN) && (*value < REF_INTERN_1V1_MAX)) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t WriteEeReferenceId() 
{
    uint16_t ee_id = eeprom_read_word((uint16_t*)(EE_ANALOG_BASE_ADDR+EE_ANALOG_ID));
    if ( eeprom_is_ready() )
    {
        uint16_t value = 0x4144; // 'A' is 0x41 and 'D' is 0x44;
        if (ee_id != value)
        {
            eeprom_write_word( (uint16_t *)(EE_ANALOG_BASE_ADDR+EE_ANALOG_ID), value);
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t WriteEeReferenceAvcc() 
{
    uint32_t ee_ref_extern_avcc_uV = eeprom_read_dword((uint32_t*)(EE_ANALOG_BASE_ADDR+EE_ANALOG_REF_EXTERN_AVCC)); 
    if ( eeprom_is_ready() )
    {
        if (ee_ref_extern_avcc_uV != ref_extern_avcc_uV)
        {
            eeprom_write_dword( (uint32_t *)(EE_ANALOG_BASE_ADDR+EE_ANALOG_REF_EXTERN_AVCC), ref_extern_avcc_uV);
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t WriteEeReference1V1() 
{
    uint32_t ee_ref_intern_1v1_uV = eeprom_read_dword((uint32_t*)(EE_ANALOG_BASE_ADDR+EE_ANALOG_REF_INTERN_1V1)); 
    if ( eeprom_is_ready() )
    {
        if (ee_ref_intern_1v1_uV != ref_intern_1v1_uV)
        {
            eeprom_write_dword( (uint32_t *)(EE_ANALOG_BASE_ADDR+EE_ANALOG_REF_INTERN_1V1), ref_intern_1v1_uV);
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t LoadAnalogRefFromEEPROM() 
{
    uint16_t id = eeprom_read_word((uint16_t*)(EE_ANALOG_BASE_ADDR+EE_ANALOG_ID));
    if (id == 0x4144) // 'A' is 0x41 and 'D' is 0x44
    {
        ref_extern_avcc_uV = eeprom_read_dword((uint32_t*)(EE_ANALOG_BASE_ADDR+EE_ANALOG_REF_EXTERN_AVCC)); 
        ref_intern_1v1_uV = eeprom_read_dword((uint32_t*)(EE_ANALOG_BASE_ADDR+EE_ANALOG_REF_INTERN_1V1));
        ref_loaded = 1;
        return 1;
    }
    else
    {
        ref_loaded = 0;
        return 0;
    }
}
