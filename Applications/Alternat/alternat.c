/*
alternat is a library that accesses the managers alternat power reporting and control.
Copyright (C) 2019 Ronald Sutherland

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES 
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE 
FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY 
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, 
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, 
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

https://en.wikipedia.org/wiki/BSD_licenses#0-clause_license_(%22Zero_Clause_BSD%22)
*/
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../lib/parse.h"
#include "../lib/rpu_mgr.h"
#include "../lib/timers_bsd.h"
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
        alternat_serial_print_started_at = milliseconds();
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
        unsigned long kRuntime= elapsed(&alternat_serial_print_started_at);
        if ((kRuntime) > (serial_print_delay_milsec))
        {
            command_done = 10; /* This keeps looping output forever (until a Rx char anyway) */
        }
    }
} 

