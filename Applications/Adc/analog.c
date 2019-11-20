/*
analog is a library that returns Analog Conversions for channels 
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

  I believe the LGPL is used in things like libraries and allows you to include them in 
  application code without the need to release the application source while GPL requires 
  that all modifications be provided as source when distributed.
*/
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <avr/eeprom.h> 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../lib/parse.h"
#include "../lib/adc.h"
#include "../lib/rpu_mgr.h"
#include "../lib/timers.h"
#include "../lib/pins_board.h"
#include "analog.h"
#include "references.h"

static unsigned long serial_print_started_at;

static uint8_t adc_arg_index;

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
        }

        if (arg_indx_channel == 4) //was ADC4 on ^0, now on manager ADC channel 1
        {
            printf_P(PSTR("\"ALT_V\":"));
        }
        
        if (arg_indx_channel == 5) //was ADC5 on ^0, now on manager ADC channel 0
        {
            printf_P(PSTR("\"ALT_I\":"));
        }

        if (arg_indx_channel == 6) //was ADC6 on ^0, now on manager ADC channel 6
        {
            printf_P(PSTR("\"PWR_I\":"));
        }
        
        if (arg_indx_channel == 7) //was ADC7 on ^0, now on manager ADC channel 7
        {
            printf_P(PSTR("\"PWR_V\":"));
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
            printf_P(PSTR("\"%1.2f\""),(analogRead(arg_indx_channel)*(ref_extern_avcc_uV/1.0E6)/1024.0));
        }

        if (arg_indx_channel == 4) //ADC4 was ALT_V on ^0, but is from manager over i2c now
        {
            printf_P(PSTR("\"%1.2f\""),(i2c_get_analogRead_from_manager(ALT_V)*((ref_extern_avcc_uV/1.0E6)/1024.0)*(110.0/10.0)));
        }

        // ADC5 was connected to a 50V/V high side current sense.
        if (arg_indx_channel == 5) //ADC5 was ALT_I on ^0, but is from manager over i2c now
        {
            printf_P(PSTR("\"%1.3f\""),(i2c_get_analogRead_from_manager(ALT_I)*((ref_extern_avcc_uV/1.0E6)/1024.0)/(0.018*50.0)));
        }

        // ADC6 was connected to a 50V/V high side current sense.
        if (arg_indx_channel == 6) //ADC6 was PWR_I on ^0, but is from manager over i2c now
        {
            printf_P(PSTR("\"%1.3f\""),(i2c_get_analogRead_from_manager(PWR_I)*((ref_extern_avcc_uV/1.0E6)/1024.0)/(0.068*50.0)));
        }

        if (arg_indx_channel == 7) //ADC7 was PWR_V on ^0, but is from manager over i2c now
        {
            printf_P(PSTR("\"%1.2f\""),(i2c_get_analogRead_from_manager(PWR_V)*((ref_extern_avcc_uV/1.0E6)/1024.0)*(115.8/15.8)));
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
