/*
  Analog-to-Digital Converter
  Copyright (c) 2016 Ronald S,. Sutherland

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

#include <util/atomic.h>
#include "adc.h"
#include "adc_bsd0.h"


// report ADC reading for a given channel.
// uses interrupt driven buffer when available
// otherwise does busy waiting
// api is a well know LGPL software, respect it as such.
int analogRead(uint8_t channel)
{
    if (ADC_auto_conversion)
    {
        ATOMIC_BLOCK ( ATOMIC_RESTORESTATE )
        {
            // this moves two byes one at a time, so the ISR could change it durring the move
            return adc[channel];
        }
    }
    else
    {
        uint8_t low, high;

#if defined(ADCSRB) && defined(MUX5)
        // the MUX5 bit of ADCSRB selects whether we're reading from channels
        // 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
        ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((channel >> 3) & 0x01) << MUX5);
#endif
      
#if defined(ADMUX)
        // clear the channel select MUX, ADLAR is not changed (0 is the default).
        uint8_t local_ADMUX = ADMUX & ~(1<<MUX3) & ~(1<<MUX2) & ~(1<<MUX1) & ~(1<<MUX0);

        // clear the reference bits REFS0, REFS1[,REFS2]
        local_ADMUX = (local_ADMUX & ~(ADREFSMASK));
        
        // select the reference
        local_ADMUX = local_ADMUX | analog_reference ;
    
        // select the channel (note MUX4 has some things for advanced users).
        ADMUX = local_ADMUX | (channel & 0x07) ;
#else
#   error missing ADMUX register which is used to sellect the reference and channel
#endif

#if defined(ADCSRA) && defined(ADCL)
        // start the conversion
        ADCSRA |= (1 <<ADSC);

        // ADSC is cleared when the conversion finishes
        while (ADCSRA & (1 <<ADSC));    

        // we have to read ADCL first; doing so locks both ADCL
        // and ADCH until ADCH is read. 
        low  = ADCL;
        high = ADCH;
#else
#   error missing ADCSRA register which has ADSC bit that is used to start a conversion
#endif

        // combine the two bytes
        return (high << 8) | low;
    }
    // this should never run.
    return -1;
}
