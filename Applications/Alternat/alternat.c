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
#include "../DayNight/day_night.h"
#include "alternat.h"

static unsigned long alternat_serial_print_started_at;

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
        uint8_t alt_enable = ( manager_status & (1<<4) )>>4; // bit 4 in status has ALT_EN value
        if (alt_enable)
        {
            printf_P(PSTR("\"ON\""));
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

// report ALT_EN, charge_start, charge_stop
void AltPwrCntl(unsigned long serial_print_delay_milsec)
{
    if ( (command_done == 10) )
    {
        alternat_serial_print_started_at = millis();
        printf_P(PSTR("{\"mgr_alt_en\":\"0x%X\","), (manager_status & (1<<4) )); // bit 4 .. alternate power enable (ALT_EN) 
        command_done = 11;
    }
    else if ( (command_done == 11) )
    {
        int local_copy = i2c_int_access_cmd(CHARGE_BATTERY_START,0);
        printf_P(PSTR("\"charge_start\":\"%u\","),local_copy);
        command_done = 12;
    }
    else if ( (command_done == 12) )
    {
        int local_copy = i2c_int_access_cmd(CHARGE_BATTERY_STOP,0);
        printf_P(PSTR("\"charge_stop\":\"%u\","),local_copy);
        command_done = 24;
    }
    else if ( (command_done == 24) )
    { 
        printf_P(PSTR("}\r\n"));
        command_done = 25;
    }
    else if ( (command_done == 25) ) 
    {
        unsigned long kRuntime= millis() - alternat_serial_print_started_at;
        if ((kRuntime) > (serial_print_delay_milsec))
        {
            command_done = 10; /* This keeps looping output forever (until a Rx char anyway) */
        }
    }
} 

