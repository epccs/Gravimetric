/*
adc_burst is how manager updates its static array of analog values  
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
#include "../lib/timers.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"
#include "../lib/adc.h"
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
        accumulate_alt_ti += analogRead(ALT_I);
        accumulate_pwr_ti += analogRead(PWR_I);
        enable_ADC_auto_conversion(BURST_MODE);
        adc_started_at += ADC_DELAY_MILSEC; 
    } 
}
