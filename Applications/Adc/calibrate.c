/*
calibrate is a library used to work with referances for Analog Conversions, 
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
#include "../lib/adc.h"
#include "../lib/timers.h"
#include "../lib/pins_board.h"
#include "references.h"
#include "calibrate.h"

// arg[0] is external voltage on AVCC pin in microVolts (10E-6)
void CalibrateAVCC(void)
{
    if ( (command_done == 10) )
    {
        // check that arg[0] starts with a digit 
        if ( ( !( isdigit(arg[0][0]) ) ) )
        {
            printf_P(PSTR("{\"err\":\"%s0NaN\"}\r\n"),command[1]);
            return;
        }
        unsigned long ul_from_arg0 = strtoul(arg[0], (char **)NULL, 10);
        if ( !IsValidValForAvccRef(&ul_from_arg0) )
        {
            printf_P(PSTR("{\"err\":\"%s0OtOfRng\"}\r\n"),command[1]);
            initCommandBuffer();
            return;
        }
        
        ref_extern_avcc_uV = ul_from_arg0;
        printf_P(PSTR("{\"REF\":{"));
        command_done = 11;
    }
    else if ( (command_done == 11) )
    {  
        printf_P(PSTR("\"extern_avcc\":\"%1.4f\","),(ref_extern_avcc_uV/1.0E6));
        command_done = 12;
    }
    else if ( (command_done == 12) )
    {
        printf_P(PSTR("}}\r\n"));
        initCommandBuffer();
    }
    else
    {
        printf_P(PSTR("{\"err\":\"DelayCmdDnWTF\"}\r\n"));
        initCommandBuffer();
    }
}

// arg[0] is internal bandgap 1V1 in microVolts
void Calibrate1V1(void)
{
    if ( (command_done == 10) )
    {
        // check that arg[0] starts with a digit 
        if ( ( !( isdigit(arg[0][0]) ) ) )
        {
            printf_P(PSTR("{\"err\":\"%s0NaN\"}\r\n"),command[1]);
            return;
        }
        unsigned long ul_from_arg0 = strtoul(arg[0], (char **)NULL, 10);
        if ( !IsValidValFor1V1Ref(&ul_from_arg0) )
        {
            printf_P(PSTR("{\"err\":\"%s0OtOfRng\"}\r\n"),command[1]);
            initCommandBuffer();
            return;
        }

        ref_intern_1v1_uV = ul_from_arg0;
        printf_P(PSTR("{\"REF\":{"));
        command_done = 11;
    }
    else if ( (command_done == 11) )
    {
        printf_P(PSTR("\"intern_1v1\":\"%1.4f\","),(ref_intern_1v1_uV/1.0E6));
        command_done = 12;
    }
    else if ( (command_done == 12) )
    {
        printf_P(PSTR("}}\r\n"));
        initCommandBuffer();
    }
    else
    {
        printf_P(PSTR("{\"err\":\"Cal1V1CmdDnWTF\"}\r\n"));
        initCommandBuffer();
    }
}


// Use with command line to load referances values from EEPROM
void ReFmEe(void)
{
    if ( (command_done == 10) )
    {
        if ( eeprom_is_ready() )
        {
            if (LoadAnalogRefFromEEPROM())
            {
                printf_P(PSTR("{\"REF\":{"));
                command_done = 11;
            }
            else
            {
                printf_P(PSTR("{\"err\":\"LdAdcEeFailNoRef\"}\r\n"));
                initCommandBuffer();
                return;
            }
        }
    }
    else if ( (command_done == 11) )
    {  
        printf_P(PSTR("\"extern_avcc\":\"%1.4f\","),(ref_extern_avcc_uV/1.0E6));
        command_done = 12;
    }
    else if ( (command_done == 12) )
    {
        printf_P(PSTR("\"intern_1v1\":\"%1.4f\","),(ref_intern_1v1_uV/1.0E6));
        command_done = 13;
    }
    else if ( (command_done == 13) )
    {
        printf_P(PSTR("}}\r\n"));
        initCommandBuffer();
    }
    else
    {
        printf_P(PSTR("{\"err\":\"LdAdcEeCmdDnWTF\"}\r\n"));
        initCommandBuffer();
    }
}

// Use with command line to save referances values to EEPROM
void Ref2Ee(void)
{
    if ( (command_done == 10) )
    {
         if ( IsValidValForAvccRef(&ref_extern_avcc_uV) && IsValidValFor1V1Ref(&ref_intern_1v1_uV) )
         {
            if ( WriteEeReferenceId() ) // may be false when eeprom is not ready
            {
                printf_P(PSTR("{\"REF\":{"));
                command_done = 11;
            }
         }
         else
        {
            printf_P(PSTR("{\"err\":\"SvFailBadRef\"}\r\n"));
            initCommandBuffer();
            return;
        }
    }
    else if ( (command_done == 11) )
    {
        if (WriteEeReferenceAvcc())
        {
            printf_P(PSTR("\"extern_avcc\":\"%1.4f\","),(ref_extern_avcc_uV/1.0E6));
            command_done = 12;
        }
    }
    else if ( (command_done == 12) )
    {
        if (WriteEeReference1V1())
        {
            printf_P(PSTR("\"intern_1v1\":\"%1.4f\","),(ref_intern_1v1_uV/1.0E6));
            command_done = 13;
        }
    }
    else if ( (command_done == 13) )
    {
        printf_P(PSTR("}}\r\n"));
        initCommandBuffer();
    }
    else
    {
        printf_P(PSTR("{\"err\":\"SvAdcEeCmdDnWTF\"}\r\n"));
        initCommandBuffer();
    }
}


