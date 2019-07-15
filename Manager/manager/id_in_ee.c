/*
id_in_ee is how RPUBUS manager checks if it has saved a address on the eeprom  
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

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "../lib/timers.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"
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
    if (eeprom_is_ready())
    {
        // up to first EE_RPU_IDMAX states may be used for writhing an ID to the EEPROM
        if ( (write_rpu_address_to_eeprom >= 1) && (write_rpu_address_to_eeprom <= EE_RPU_IDMAX) )
        { // write "RPUadpt\0" at address EE_RPU_ID
            uint8_t value = pgm_read_byte(&EE_IdTable[write_rpu_address_to_eeprom-1]);
            eeprom_write_byte( (uint8_t *)((write_rpu_address_to_eeprom-1)+EE_RPU_ID), value);
            
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
            eeprom_write_byte( (uint8_t *)(EE_RPU_ADDRESS), value);
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
