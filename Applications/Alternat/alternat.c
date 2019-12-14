/*
 alternat is a library that accesses the managers alternat power reporting and control.
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
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../lib/parse.h"
#include "../lib/adc.h"
#include "../lib/timers.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"
#include "../lib/rpu_mgr.h"
#include "../Adc/references.h"
#include "alternat.h"


// the manager status byte can be read with i2c command 6. 
// status bit 4 has ALT_EN
void EnableAlt(void)
{
    if ( (command_done == 10) )
    {
        printf_P(PSTR("{\"alt_en\":"));
        command_done = 11;
    }
    else if ( (command_done == 11) )
    {
        uint8_t status = i2c_read_status();
        uint8_t alt_enable = ( status & (1<<4) )>>4; // bit 4 in status has ALT_EN value
        if (alt_enable)
        {
            printf_P(PSTR("\"ON\""));
            alt_count = 0;
        }
        else
        {
            printf_P(PSTR("\"OFF\""));
        }
        command_done = 12;
    }
    else if ( (command_done == 12) )
    {
        printf_P(PSTR("}\r\n"));
        initCommandBuffer();
    }
    else
    {
        initCommandBuffer();
    }
}

/*
void AltCount(void)
{
    if ( (command_done == 10) )
    {
        printf_P(PSTR("{\"alt_count\":"));
        command_done = 11;
    }
    else if ( (command_done == 11) )
    {
        printf_P(PSTR("\"%u\""), alt_count); 
        command_done = 12;
    }
    else if ( (command_done == 12) )
    { 
        printf_P(PSTR("}\r\n"));
        initCommandBuffer();
    }
    else
    {
        initCommandBuffer();
    }
} 
*/

