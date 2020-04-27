/*
calibration limits is a library used to load and set analog channel corrections in EEPROM. 
Copyright (C) 2020 Ronald Sutherland

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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <avr/eeprom.h> 
#include "calibration_limits.h"

volatile uint8_t cal_loaded;
volatile uint8_t adc_enum_with_writebit;

struct Cal_Map calMap[ADC_ENUM_END];

// check if calibration is a valid float
// 0UL and 0xFFFFFFFFUL are not valid
uint8_t IsValidValForCal(ADC_ENUM_t cal_map)
{
    if (cal_map > ADC_ENUM_END)
    {
        return 0;
    }
    uint32_t tmp_cal;
    memcpy(&tmp_cal, &calMap[cal_map].calibration, sizeof tmp_cal);
    if ( (tmp_cal == 0xFFFFFFFFUL) | (tmp_cal == 0x0UL) )
    {
            return 0;
    }
    return 1;
}


// check if adc_enum_with_writebit calibration is valid
uint8_t IsValidValForCalChannel(void) 
{
    uint8_t adc_enum  = adc_enum_with_writebit & CAL_CHANNEL_MASK; // mask the writebit
    if ( (adc_enum < ADC_ENUM_END) )
    {
        // copy the float to a uint32_t to check if bytes are valid, 
        // python:  unpack('f', pack('BBBB', 0xff, 0xff, 0xff, 0xff)) is (nan,)
        // eeprom that is not yet set is 0xff and should be ignored if passed through i2c
        return IsValidValForCal(adc_enum);
            
    }
    return 0;
}

// save channel calibration if writebit is set and eeprom is ready
uint8_t WriteCalToEE(void)
{
    if (adc_enum_with_writebit & 0x80)
    {
        uint8_t adc_enum  = adc_enum_with_writebit & 0x7F; // mask the writebit 
        // use update functions to skip the burning if the old value is the same with new.
        // https://www.microchip.com/webdoc/AVRLibcReferenceManual/group__avr__eeprom.html
        if ( eeprom_is_ready() )
        {
            eeprom_update_float( (float *)(EE_CAL_BASE_ADDR+(EE_CAL_OFFSET*adc_enum)), calMap[adc_enum].calibration);
            adc_enum_with_writebit = adc_enum;
            return 1;
        }
    }
    return 0;
}

// load a channel calibraion from EEPROM or set default if not valid (0 or 0xFFFFFFFF are not valid for calibration).
// Befor loading calibraions from EEPROM set 
// cal_loaded = CAL_CLEAR; 
// then loop load calibraion for each enum in ADC_ENUM_t
void LoadCalFromEEPROM(ADC_ENUM_t cal_map) 
{
    if (cal_map < ADC_ENUM_END) // ignore out of range
    {
        calMap[cal_map].calibration = eeprom_read_float((float *)(EE_CAL_BASE_ADDR+(EE_CAL_OFFSET*cal_map)));
        if ( !IsValidValForCal(cal_map) ) 
        {
            if (cal_map == ADC_ENUM_ALT_I) //channelToCalMap[ADC_CH_ALT_I].cal_map
            {
                calMap[cal_map].calibration = (1.0/(1<<10))/(0.018*50.0); // ALT_I has  0.018 Ohm sense resistor and gain of 50
                cal_loaded = cal_loaded + CAL_0_DEFAULT;
            }
            if (cal_map == ADC_ENUM_ALT_V) // channelToCalMap[ADC_CH_ALT_V].cal_map
            {
                calMap[cal_map].calibration = (1.0/(1<<10))*((100+10.0)/10.0); // ALT_V has divider with 100k and 10.0k
                cal_loaded = cal_loaded + CAL_1_DEFAULT;
            }
            if (cal_map == ADC_ENUM_PWR_I) // channelToCalMap[ADC_CH_PWR_I].cal_map
            {
                calMap[cal_map].calibration = (1.0/(1<<10))/(0.068*50.0); // PWR_I has 0.068 Ohm sense resistor and gain of 50
                cal_loaded = cal_loaded + CAL_2_DEFAULT;
            }
            if (cal_map == ADC_ENUM_PWR_V) // channelToCalMap[ADC_CH_PWR_V].cal_map
            {
                calMap[cal_map].calibration = (1.0/(1<<10))*((100+15.8)/15.8); // PWR_V has divider with 100k and 15.8k
                cal_loaded = cal_loaded + CAL_3_DEFAULT;
            }
        }
        else
        {
            // calibration from EEPROM is valid, so it has been kept. It is not a default value so
            // clear the CAL_n_DEFAULT bit (0..3) of cal_loaded
            uint8_t mask_for_cal_default_bit = ~(1<<cal_map);
            cal_loaded = cal_loaded & mask_for_cal_default_bit; // now clear the CAL_n_DEFAULT bit
        }
    }
}

// save channel calibration from I2C to EEPROM (if valid)
void ChannelCalFromI2CtoEE(void)
{
    if (cal_loaded & (CAL_0_TOSAVE | CAL_1_TOSAVE | CAL_2_TOSAVE | CAL_3_TOSAVE) )
    {
        // adc_enum_with_writebit agree (e.g., writebit set)
        if (adc_enum_with_writebit & 0x80)
        {
            uint8_t adc_enum  = adc_enum_with_writebit & 0x7F; // mask the writebit 
            // does the channelToCalMap.cal_map agree, e.g., CAL_0_TOSAVE is 0x10
            // ALT_I is 0; adc_enum is 0; finaly (1<<0) == 0x10>>4
            if ( (1<<adc_enum) == (cal_loaded>>4) )
            {
                // final check befor trying to save
                if ( IsValidValForCalChannel() )
                {
                    if (WriteCalToEE())
                    {
                        // clear the CAL_n_TOSAVE bits
                        cal_loaded = cal_loaded & 0x0F;
                        // also clear the correct CAL_n_DEFAULT bit (calibration is not default)
                        uint8_t mask_for_cal_default_bit = ~(1<<adc_enum);
                        cal_loaded = cal_loaded & mask_for_cal_default_bit; // now clear the CAL_n_DEFAULT bit
                        return; // all done
                    }
                }
                else
                {
                    LoadCalFromEEPROM((ADC_ENUM_t) adc_enum); // ignore value since it is not valid
                }
            }       
        }
    }
}
