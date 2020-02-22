/*
Interrupt-Driven Analog-to-Digital Converter
Copyright (C) 2020 Ronald Sutherland

All rights reserved, specifically, the right to distribute is withheld. Subject to your compliance 
with these terms, you may use this software and any derivatives exclusively with Ronald Sutherland 
products. It is your responsibility to comply with third party license terms applicable to your use 
of third party software (including open source software) that accompany Ronald Sutherland software.

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

how to use the buffer

    int reading;
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE )
    {
        // this moves two byes one at a time, so the ISR could change it durring the move
        reading = adc[channel];
    }
*/

#include <util/atomic.h>
#include "adc_bsd.h"

//**** must not use LGPL 
#include "pin_num.h"
#include "pins_board.h"

volatile int adc[ADC_CHANNELS];
volatile uint8_t adc_channel;
volatile uint8_t ADC_auto_conversion;
volatile uint8_t analog_reference;
volatile uint8_t adc_isr_status;

static uint8_t free_running;

// Interrupt service routine for enable_ADC_auto_conversion
ISR(ADC_vect){
    adc[adc_channel] = ADC;
    
    ++adc_channel;
    
    // ch 0 is ALT_I, always read
    // ch 1 is ALT_V, only read when ALT_EN is low (e.g., at rest/not charging)
    if ( (adc_channel == 1) && digitalRead(ALT_EN) )
    {
        adc_channel = 6; // skip channel 1
    }
    // skip channels 3..5
    if (adc_channel == 2)
    {
        adc_channel = 6;
    }
    // ch 6 is PWR_I, always read
    // ch 7 is PWR_V, only read when ALT_EN is low (e.g., at rest/not charging)
    if ( (adc_channel == 7) && digitalRead(ALT_EN) )
    {
        adc_channel = 8; // skip channel 7
    }
    
    if (adc_channel >= ADC_CHANNELS) 
    {
        adc_channel = 0;
        adc_isr_status = ISR_ADCBURST_DONE; // mark to notify burst is done
        if (!free_running)
        {
            return;
        }

    }

#if defined(ADMUX)
    // clear the mux to select the next channel to do conversion without changing the reference
    ADMUX &= ~(1<<MUX3) & ~(1<<MUX2) & ~(1<<MUX1) & ~(1<<MUX0);
        
    // use a stack register to reset the referance, most likly it is not changed and fliping the hardware bit would mess up the reading.
    ADMUX = ( (ADMUX & ~(ADREFSMASK) & ~(1<<ADLAR) ) | analog_reference ) + adc_channel;
#else
#   error missing ADMUX register which is used to sellect the reference and channel
#endif

    // set ADSC in ADCSRA, ADC Start Conversion
    ADCSRA |= (1<<ADSC);
}


// select a referance (EXTERNAL_AVCC, INTERNAL_1V1) and initialize ADC
void init_ADC_single_conversion(uint8_t reference)
{
    // The user must select the reference they want to initialization the ADC with, 
    // it should not be automagic. Smoke will get let out if AREF is connected to
    // another source while AVCC is selected. AREF should not be run to a user pin.
    analog_reference = reference;
    free_running = 0;

#if defined(ADMUX)
    // clear the channel select MUX
    uint8_t local_ADMUX = ADMUX & ~(1<<MUX3) & ~(1<<MUX2) & ~(1<<MUX1) & ~(1<<MUX0);

    // clear the reference bits REFS0, REFS1[,REFS2]
    local_ADMUX = (local_ADMUX & ~(ADREFSMASK));
    
    // select the reference so it has time to stabalize.
    ADMUX = local_ADMUX | reference ;
#else
#   error missing ADMUX register which is used to sellect the reference and channel
#endif
    
// On most avr5 core chips the adc_clock needs to run between 50kHz<adc_clock<200kHz
// for maximum resolution
#if defined(ADCSRA)
	#if (50000 < F_CPU/128) && (F_CPU/128 < 200000 ) // set prescaler /128
		ADCSRA |= (1<<ADPS2);
		ADCSRA |= (1<<ADPS1);
		ADCSRA |= (1<<ADPS0);
	#elif (50000 < F_CPU/64) && (F_CPU/64 < 200000 ) // set prescaler /64
		ADCSRA |= (1<<ADPS2);
		ADCSRA |= (1<<ADPS1);
		ADCSRA &= ~(1<<ADPS0);
	#elif (50000 < F_CPU/32) && (F_CPU/32 < 200000 ) // set prescaler /32
		ADCSRA |= (1<<ADPS2);
		ADCSRA &= ~(1<<ADPS1);
		ADCSRA |= (1<<ADPS0);
	#elif (50000 < F_CPU/16) && (F_CPU/16 < 200000 ) // set prescaler /16
		ADCSRA |= (1<<ADPS2);
		ADCSRA &= ~(1<<ADPS1);
		ADCSRA &= ~(1<<ADPS0);
	#elif (50000 < F_CPU/8) && (F_CPU/8 < 200000 ) // set prescaler /8
		ADCSRA &= ~(1<<ADPS2);
		ADCSRA |= (1<<ADPS1);
		ADCSRA |= (1<<ADPS0);
	#elif (50000 < F_CPU/4) && (F_CPU/4 < 200000 ) // set prescaler /4
		ADCSRA &= ~(1<<ADPS2);
		ADCSRA |= (1<<ADPS1);
		ADCSRA &= ~(1<<ADPS0);
	#elif (50000 < F_CPU/2) && (F_CPU/2 < 200000 ) // set prescaler /2. ADPS[2:0] == b001 or b000 gives same prescaler
		ADCSRA &= ~(1<<ADPS2);
		ADCSRA &= ~(1<<ADPS1);
        ADCSRA |= (1<<ADPS0);
    #else 
    #   error can not set adc_clock for maximum resolution
	#endif
	ADCSRA |= (1<<ADEN); // enable adc conversion
#else
#   error missing ADCSRA register which is used to set the prescaler range
#endif
    ADC_auto_conversion = 0; 
}


// Before setting the ADC Auto Trigger mode, use init_ADC_single_conversion 
// to select reference and set the adc_clock pre-scaler. This call will start 
// taking readings on each channel the ISR iterates over and holds the result 
// in a buffer.
void enable_ADC_auto_conversion(uint8_t free_run)
{
    adc_channel = 0;
    adc_isr_status = ISR_ADCBURST_START; // mark so we know new readings are arriving
    free_running = free_run;

#if defined(ADCSRA)
	// Power up the ADC and set it for conversion with interrupts enabled
    ADCSRA = ( (ADCSRA | (1<<ADEN) ) & ~(1<<ADATE) ) | (1 << ADIE);

    // Start the first Conversion (ISR will start each one after the previous is done)
    ADCSRA |= (1<<ADSC);
#else
#   error missing ADCSRA register which has ADSC bit that is used to start a conversion
#endif
    ADC_auto_conversion =1;
}