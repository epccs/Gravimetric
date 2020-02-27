/*
id_in_ee is how RPUBUS manager checks if it has saved a address on the eeprom  
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

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "rpubus_manager_state.h"
#include "id_in_ee.h"

const uint8_t EE_IdTable[] PROGMEM =
{
    'R',
    'P',
    'U',
    'i',
    'd',
    '\0' // null term
};

void save_rpu_addr_state(void)
{
    if (write_rpu_address_to_eeprom && eeprom_is_ready())
    {
        // up to first EE_RPU_IDMAX locations may be used for writing an ID to the EEPROM
        if ( (write_rpu_address_to_eeprom >= 1) && (write_rpu_address_to_eeprom <= EE_RPU_IDMAX) )
        { // write "RPUid\0" at address EE_RPU_ID
            uint8_t value = pgm_read_byte(&EE_IdTable[write_rpu_address_to_eeprom-1]);
            eeprom_update_byte( (uint8_t *)((write_rpu_address_to_eeprom-1)+EE_RPU_ID), value);
            
            if (value == '\0') 
            {
                write_rpu_address_to_eeprom = 11;
            }
            else
            {
                write_rpu_address_to_eeprom += 1;
            }
        }
        
        if ( (write_rpu_address_to_eeprom == 11) )
        { // write the rpu address to eeprom address EE_RPU_ADDRESS 
            uint8_t value = rpu_address;
            eeprom_update_byte( (uint8_t *)(EE_RPU_ADDRESS), value);
            write_rpu_address_to_eeprom = 0;
        }
    }
}

// check if eeprom ID is valid
uint8_t check_for_eeprom_id(void)
{
    uint8_t EE_id_valid = 0;
    for(uint8_t i = 0; i <EE_RPU_IDMAX; i++)
    {
        uint8_t id = pgm_read_byte(&EE_IdTable[i]);
        uint8_t ee_id = eeprom_read_byte((uint8_t*)(i+EE_RPU_ID)); 
        if (id != ee_id) 
        {
            EE_id_valid = 0;
            break;
        }
        
        if (id == '\0') 
        {
            EE_id_valid = 1;
            break;
        }
    }
    return EE_id_valid;
}
