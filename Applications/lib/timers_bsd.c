/*
Initialize AVR Timers  
Copyright (c) 2020 Ronald S,. Sutherland

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

#include <util/atomic.h>
#include <avr/interrupt.h>
#include "timers_bsd.h"

volatile uint32_t tick = 0;
static uint32_t millisec_tick_last_used = 0;
static uint16_t uS_balance = 0;
static unsigned long millisec = 0;

ISR(TIMER0_OVF_vect)
{
    // swap to local since volatile has to be read from memory on every access
    uint32_t local_tick = tick;
    ++local_tick;
    tick = local_tick;
}

/* setup Timer0: /64 Fast PWM
         Timer1: /64 Phase Correct, 8-bit
         Timer2: /64 phase-correct PWM
         Timer3, Timer4, Timer5: /64 Phase Correct, 8-bit */
void initTimers()
{
    // Timer0 is set for fast PWM (not phase-correct)
#if defined(TCCR0A) && defined(WGM00) && defined(WGM01) && defined(WGM02)
    // WGM0[2:0] set 0b011 for 328pb or 324pb
    uint8_t local_TCCR0A = (TCCR0A & ~(1<<WGM02)) | (1<<WGM01) | (1<<WGM00);
    TCCR0A = local_TCCR0A;
#else
    #error Timer0 Fast PWM Mode of Operation not available
#endif

    // Timer0 clock select clk/64
#if defined(TCCR0B) && defined(CS00) && defined(CS01) && defined(CS02)
    // CS0[2:0] set 0b011 for 328pb or 324pb
    uint8_t local_TCCR0B = (TCCR0B & ~(1<<CS02)) | (1<<CS01) | (1<<CS00);
    TCCR0B = local_TCCR0B;
#else
    #error Timer0 clock select and prescale factor (\64) not available
#endif

    // Timer0 interrupt 
    TIMSK0 |= (1<<TOIE0);

    // Timers1 is /64, 16 bit, but set for 8 bit phase-correct PWM
#if defined(TCCR1A) && defined(TCCR1B) && defined(CS11) && defined(CS10) && defined(WGM10)
    // COM1A[1:0] and COM1B[1:0] set OC1A/OC1B disconnected.
    TCCR1A = 0; 
    // WGM1[2:0] set 0b001 Timer1 Mode of Operation
    TCCR1A |= (1<<WGM10); //PWM, Phase Correct, 8-bit

    TCCR1B = 0;

    // CS1[2:0] set 0b011 Timer1 clock select clk/64
    TCCR1B |= (1<<CS11) | (1<<CS10);
#else
    #error Timer1 clock select and prescale factor (\64) not available
#endif

    // Timer2 is /64 and set for phase-correct PWM
#if defined(TCCR2A) && defined(TCCR2B) && defined(CS22) && defined(WGM20)
    // COM2A[1:0] and COM2B[1:0] set OC2A/OC2B disconnected.
    TCCR2A = 0;
    // WGM2[2:0] set 0b001 Timer1 Mode of Operation
    TCCR2A |= (1<<WGM20); //PWM, Phase Correct

    TCCR2B = 0;
    // CS2[2:0] set 0b100 Timer1 clock select clk/64
    TCCR2B |= (1<<CS22);
#else
    #error Timer2 clock select and prescale factor (\64) not available
#endif

// Timers3 is /64, 16 bit, but set for 8 bit phase-correct PWM
#if defined(TCCR3A) && defined(TCCR3B) && defined(CS31) && defined(CS30) && defined(WGM30)
    // COM3A[1:0] and COM3B[1:0] set OC3A/OC3B disconnected.
    TCCR3A = 0; 
    // WGM3[2:0] set 0b001 Timer3 Mode of Operation
    TCCR3A |= (1<<WGM30); //PWM, Phase Correct, 8-bit

    TCCR3B = 0;

    // CS3[2:0] set 0b011 Timer3 clock select clk/64
    TCCR3B |= (1<<CS31) | (1<<CS30);
#endif

// Timers4 is /64, 16 bit, but set for 8 bit phase-correct PWM
#if defined(TCCR4A) && defined(TCCR4B) && defined(CS41) && defined(CS40) && defined(WGM40)
    // COM4A[1:0] and COM4B[1:0] set OC4A/OC4B disconnected.
    TCCR4A = 0; 
    // WGM4[2:0] set 0b001 Timer4 Mode of Operation
    TCCR4A |= (1<<WGM40); //PWM, Phase Correct, 8-bit

    TCCR4B = 0;

    // CS4[2:0] set 0b011 Timer4 clock select clk/64
    TCCR4B |= (1<<CS41) | (1<<CS40);
#endif

// Timers5 is /64, 16 bit, but set for 8 bit phase-correct PWM
#if defined(TCCR5A) && defined(TCCR5B) && defined(CS51) && defined(CS50) && defined(WGM50)
    // COM5A[1:0] and COM5B[1:0] set OC5A/OC5B disconnected.
    TCCR5A = 0; 
    // WGM5[2:0] set 0b001 Timer5 Mode of Operation
    TCCR5A |= (1<<WGM50); //PWM, Phase Correct, 8-bit

    TCCR5B = 0;

    // CS5[2:0] set 0b011 Timer5 clock select clk/64
    TCCR5B |= (1<<CS51) | (1<<CS50);
#endif
}

// returns a uint32 count of Timer0 overflow events.
// each tick is (64 * 256) = 16,384 crystal counts or 1.024mSec
uint32_t tickAtomic()
{
    uint32_t local;
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE )
    {
        local = tick;
    }

    return local;
}

// return the elapsed milliseconds given a pointer to a past time
unsigned long elapsed(unsigned long *past)
{
    unsigned long now = milliseconds(); //this will convert TIMER0_OVF ticks into corrected time
    return now - *past;
}

// calculate milliseconds based on the TIMER0_OVF tick  
// call every 250 ticks or the time will not be correct
unsigned long milliseconds(void)
{
    uint32_t now_tick;
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE )
    {
        now_tick = tick; //tick is volatile so get a copy to work with
    }

    // differance between now and last tick used
    uint32_t ktick = now_tick - millisec_tick_last_used;
    uint8_t ktick_byt;
    if (ktick > 250) //limit looping delays
    {
         ktick_byt = 250;
    }
    else ktick_byt = (uint8_t) ktick;
    if (ktick_byt)
    {
        while( ktick_byt ) 
        {
            // update millisec time
            ++millisec;
            ktick_byt--; 
            uS_balance  = uS_balance + MICROSEC_TICK_CORRECTION; // tick is a perfect unit of time in microseconds
            if (uS_balance > 1000) 
            {
                uS_balance = uS_balance - 1000;
                ++millisec; // add leap millisecond because tick is a perfect unit of time in microseconds which added up.
            }
        }
        millisec_tick_last_used = now_tick;
    }
    return millisec;
}


// after 2**32 counts of the tick value it will role over, e.g. 2**(14+32) crystal counts. 
// (2**(14+32))/16000000/3600/24 = 50.9 days

// Note a capture is 16 bits, and extending it has proven to be a problem. 
// it may be possible to merge the capture value and tick value to form a 46 bit event time. 
