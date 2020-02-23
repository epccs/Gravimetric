/*
analog is a library that returns Analog Conversions for channels 
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
#include "../lib/adc_bsd.h"
#include "../lib/rpu_mgr.h"
#include "../lib/timers.h"
#include "analog.h"
#include "references.h"

static unsigned long serial_print_started_at;

static uint8_t adc_arg_index;

static int temp_adc;

/* return adc values */
void Analog(unsigned long serial_print_delay_milsec)
{
    if ( (command_done == 10) )
    {
        // check that arguments are digit in the range 0..7
        for (adc_arg_index=0; adc_arg_index < arg_count; adc_arg_index++) 
        {
            if ( ( !( isdigit(arg[adc_arg_index][0]) ) ) || (atoi(arg[adc_arg_index]) < ADC0) || (atoi(arg[adc_arg_index]) > ADC_CHANNELS) )
            {
                printf_P(PSTR("{\"err\":\"AdcChOutOfRng\"}\r\n"));
                initCommandBuffer();
                return;
            }
        }
        // load reference calibration or show an error if they are not in eeprom
        if ( ! LoadAnalogRefFromEEPROM() )
        {
            printf_P(PSTR("{\"err\":\"AdcRefNotInEeprom\"}\r\n"));
            initCommandBuffer();
            return;
        }

        // print in steps otherwise the serial buffer will fill and block the program from running
        serial_print_started_at = millis();
        printf_P(PSTR("{"));
        adc_arg_index= 0;
        command_done = 11;
    }
    else if ( (command_done == 11) )
    { // use the channel as an index in the JSON reply
        uint8_t arg_indx_channel =atoi(arg[adc_arg_index]);
        
        if ( (arg_indx_channel == ADC0) || (arg_indx_channel == ADC1) || (arg_indx_channel == ADC2) || (arg_indx_channel == ADC3) )
        {
            printf_P(PSTR("\"ADC%s\":"),arg[adc_arg_index]);
            ATOMIC_BLOCK ( ATOMIC_RESTORESTATE )
            {
                temp_adc = adc[arg_indx_channel]; // this moves two byes one at a time, so the ISR could change it durring the move
            }
            
        }

        if (arg_indx_channel == 4) //was ADC4 on ^0, now on manager ADC channel 1
        {
            printf_P(PSTR("\"ALT_V\":"));
            temp_adc = i2c_get_adc_from_manager(ALT_V);
        }
        
        if (arg_indx_channel == 5) //was ADC5 on ^0, now on manager ADC channel 0
        {
            printf_P(PSTR("\"ALT_I\":"));
            temp_adc = i2c_get_adc_from_manager(ALT_I);
        }

        if (arg_indx_channel == 6) //was ADC6 on ^0, now on manager ADC channel 6
        {
            printf_P(PSTR("\"PWR_I\":"));
            temp_adc = i2c_get_adc_from_manager(PWR_I);
        }
        
        if (arg_indx_channel == 7) //was ADC7 on ^0, now on manager ADC channel 7
        {
            printf_P(PSTR("\"PWR_V\":"));
            temp_adc = i2c_get_adc_from_manager(PWR_V);
        }
        command_done = 12;
    }
    else if ( (command_done == 12) )
    {
        uint8_t arg_indx_channel =atoi(arg[adc_arg_index]);

        // There are values from 0 to 1023 for 1024 slots where each reperesents 1/1024 of the reference. Last slot has issues
        // https://forum.arduino.cc/index.php?topic=303189.0 
        // The BSS138 level shift will block voltages over 4.5V
        if ( (arg_indx_channel == ADC0) || (arg_indx_channel == ADC1) || (arg_indx_channel == ADC2) || (arg_indx_channel == ADC3))
        {
            printf_P(PSTR("\"%1.2f\""),(temp_adc*(ref_extern_avcc_uV/1.0E6)/1024.0));
        }

        if (arg_indx_channel == 4) //ADC4 was ALT_V on ^0, but is from manager over i2c now
        {
            printf_P(PSTR("\"%1.2f\""),(temp_adc*((ref_extern_avcc_uV/1.0E6)/1024.0)*(110.0/10.0)));
        }

        // ADC5 was connected to a 50V/V high side current sense.
        if (arg_indx_channel == 5) //ADC5 was ALT_I on ^0, but is from manager over i2c now
        {
            printf_P(PSTR("\"%1.3f\""),(temp_adc*((ref_extern_avcc_uV/1.0E6)/1024.0)/(0.018*50.0)));
        }

        // ADC6 was connected to a 50V/V high side current sense.
        if (arg_indx_channel == 6) //ADC6 was PWR_I on ^0, but is from manager over i2c now
        {
            printf_P(PSTR("\"%1.3f\""),(temp_adc*((ref_extern_avcc_uV/1.0E6)/1024.0)/(0.068*50.0)));
        }

        if (arg_indx_channel == 7) //ADC7 was PWR_V on ^0, but is from manager over i2c now
        {
            printf_P(PSTR("\"%1.2f\""),(temp_adc*((ref_extern_avcc_uV/1.0E6)/1024.0)*(115.8/15.8)));
        }

        if ( (adc_arg_index+1) >= arg_count) 
        {
            printf_P(PSTR("}\r\n"));
            command_done = 13;
        }
        else
        {
            printf_P(PSTR(","));
            adc_arg_index++;
            command_done = 11;
        }
    }
    else if ( (command_done == 13) ) 
    { // delay between JSON printing
        unsigned long kRuntime= millis() - serial_print_started_at;
        if ((kRuntime) > (serial_print_delay_milsec))
        {
            command_done = 10; /* This keeps looping output forever (until a Rx char anyway) */
        }
    }
    else
    {
        initCommandBuffer();
    }
}
