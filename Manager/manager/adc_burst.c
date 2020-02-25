/*
adc_burst is how manager updates its static array of analog values  
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

#include <util/atomic.h>
#include <avr/io.h>
#include "../lib/timers.h"
#include "../lib/io_enum_bsd.h"
#include "../lib/adc_bsd.h"
#include "adc_burst.h"

unsigned long adc_started_at;
unsigned long accumulate_alt_ti;
unsigned long accumulate_pwr_ti;

// every 10 mSec accumulate current (for Amp Hr) and scan the ADC channels
// high side curr sense for pwr_i is from 0.068 ohm, the adc reads 512 with 0.735 Amp
// sampling data for an hour should give 735mAHr
// ref_extern_avcc = 5.0; accumulate_pwr_ti = 512*(100 smp per Sec) * 3600 ( Sec per Hr)
// accumulate_pwr_ti*((ref_extern_avcc)/1024.0)/(0.068*50.0)/360 is in mAHr 
void adc_burst(void)
{
    unsigned long kRuntime= millis() - adc_started_at;
    if ((kRuntime) > ((unsigned long)ADC_DELAY_MILSEC))
    {
        ATOMIC_BLOCK ( ATOMIC_RESTORESTATE )
        {
            // this works with lots of byes at a time, and the ISR can change them at any time
            accumulate_alt_ti += adc[MCU_IO_ALT_I];
            accumulate_pwr_ti += adc[MCU_IO_PWR_I];
        }
        enable_ADC_auto_conversion(BURST_MODE);
        adc_started_at += ADC_DELAY_MILSEC; 
    } 
}
