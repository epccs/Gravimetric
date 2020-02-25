/*
references is a library used to load and set analog conversion references in EEPROM. 
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
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <avr/eeprom.h> 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "references.h"

volatile uint8_t ref_loaded;
volatile uint8_t ref_select_with_writebit;

struct Ref_Map refMap[REFERENCE_OPTIONS];

// check if reference is a valid float
// 0UL and 0xFFFFFFFFUL are not valid
uint8_t IsValidValForRef(REFERENCE_t ref_map) 
{
        if (ref_map > REFERENCE_OPTIONS)
    {
        return 0;
    }
    uint32_t tmp_ref;
    memcpy(&tmp_ref, &refMap[ref_map], sizeof tmp_ref);
    if ( (tmp_ref == 0xFFFFFFFFUL) | (tmp_ref == 0x0UL) )
    {
            return 0;
    }
    return 1;
}

// save channel reference if writebit is set and eeprom is ready
uint8_t WriteRefToEE() 
{
    if (ref_select_with_writebit & 0x80)
    {
        uint8_t select  = ref_select_with_writebit & 0x7F; // mask the writebit to select the reference
        // use update functions to skip the burning if the old value is the same with new.
        // https://www.microchip.com/webdoc/AVRLibcReferenceManual/group__avr__eeprom.html
        if ( eeprom_is_ready() )
        {
            eeprom_update_float( (float *)(EE_REF_BASE_ADDR+(EE_REF_OFFSET*select)), refMap[select].reference);
            ref_select_with_writebit = select;
            return 1;
        }
    }
    return 0;
}

// load a reference from EEPROM or set default if not valid (0 or 0xFFFFFFFF are not valid).
// Befor loading references from EEPROM set 
// ref_loaded = REF_CLEAR; 
// then loop load reference for each enum in REFERENCE_t
void LoadRefFromEEPROM(REFERENCE_t ref_map) 
{
    if (ref_map < REFERENCE_OPTIONS) // ignore if out of range
    {
        refMap[ref_map].reference = eeprom_read_float((float *)(EE_REF_BASE_ADDR+(EE_REF_OFFSET*ref_map)));
        if ( !IsValidValForRef(ref_map) ) 
        {
            if (ref_map == REFERENCE_EXTERN_AVCC)
            {
                refMap[ref_map].reference = 5.0;
                ref_loaded = ref_loaded + REF_0_DEFAULT;
            }
            if (ref_map == REFERENCE_INTERN_1V1)
            {
                refMap[ref_map].reference = 1.08; 
                ref_loaded = ref_loaded + REF_1_DEFAULT;
            }
        }
        else
        {
            // reference from EEPROM is valid, so it has been kept. It is not a default value so
            // clear the REF_n_DEFAULT bit (0..1) of ref_loaded
            uint8_t mask_for_ref_default_bit = ~(1<<ref_map);
            ref_loaded = ref_loaded & mask_for_ref_default_bit; // now clear the REF_n_DEFAULT bit
        }
    }
}

// save referances from I2C to EEPROM (if valid)
void ReferanceFromI2CtoEE(void)
{
    if (ref_loaded & (REF_0_TOSAVE | REF_1_TOSAVE ) )
    {
        // ref_select_with_writebit agree (e.g., writebit set)
        if (ref_select_with_writebit & 0x80)
        {
            uint8_t select = ref_select_with_writebit & 0x7F;

            // final check befor trying to save
            if ( IsValidValForRef( (REFERENCE_t) select) )
            {
                if (WriteRefToEE())
                {
                    // clear the REF_n_TOSAVE bits
                    ref_loaded = ref_loaded & 0x0F;
                    // also clear the correct REF_n_DEFAULT bit (referance is not default)
                    uint8_t mask_for_ref_default_bit = ~(1<<select);
                    ref_loaded = ref_loaded & mask_for_ref_default_bit; // now clear the REF_n_DEFAULT bit
                    return; // all done
                }
            }
            else
            {
                LoadRefFromEEPROM((REFERENCE_t) select); // ignore value since it is not valid
            }
   
        }
    }
}
