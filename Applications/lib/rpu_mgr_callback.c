/*
Manager callback functions are done over the i2c interface between application controller and manager
Copyright (C) 2020 Ronald Sutherland

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

#include <stdbool.h>
#include <string.h>
#include <avr/io.h>
#include "../lib/twi0_bsd.h"
#include "rpu_mgr_callback.h"

uint8_t i2c0Buffer[I2C_BUFFER_LENGTH];
uint8_t i2c0BufferLength = 0;
volatile uint8_t twi_slave_errorCode;

// called when I2C data is received. 
void receive_i2c_event(uint8_t* inBytes, uint8_t numBytes) 
{
    // table of pointers to functions that are selected by the i2c cmmand byte
    static void (*pf[GROUP][MGR_CMDS])(uint8_t*) = 
    {
        {fnNull, fnDayNightState, fnDayWork, fnNightWork, fnNull, fnNull, fnNull, fnNull}
    };

    // i2c will echo's back what was sent (plus modifications) with transmit event
    uint8_t i;
    for(i = 0; i < numBytes; ++i)
    {
        i2c0Buffer[i] = inBytes[i];    
    }
    if(i < I2C_BUFFER_LENGTH) i2c0Buffer[i+1] = 0; // room for null
    i2c0BufferLength = numBytes;

    // my i2c commands size themselfs with data, so at least two bytes (e.g., cmd + one_data_byte)
    if(i2c0BufferLength <= 1) 
    {
        i2c0Buffer[0] = 0xFF; // error code for small size.
        return; // not valid, do nothing just echo the error.
    }

    // mask the group bits (4..7) so they are alone then roll those bits to the left so they can be used as an index.
    uint8_t group;
    group = (i2c0Buffer[0] & 0xF0) >> 4;
     if(group >= GROUP) 
     {
        i2c0Buffer[0] = 0xFE; // error code for bad group (0xF[1..F] is error group).
        return; 
     }

    // mask the command bits (0..3) so they can be used as an index.
    uint8_t command;
    command = i2c0Buffer[0] & 0x0F;
    if(command >= MGR_CMDS) 
    {
        i2c0Buffer[0] = 0xFD; // error code for bad command.
        return; // not valid, do nothing but echo error code.
    }

    /* Call the command function and return */
    (* pf[group][command])(i2c0Buffer);
    return;	
}

void transmit_i2c_event(void) 
{
    // respond with an echo of the last message sent
    twi_slave_errorCode = twi0_fillSlaveTxBuffer(i2c0Buffer, i2c0BufferLength);
    // if (return_code != 0) status_byt &= (1<<DTR_I2C_TRANSMIT_FAIL);
}

// Callbacks need registered to accepet the data from manager's event 
// but this will initalize the events in case they are not used.
void twi0_callback_default(uint8_t data)
{
    // In a real callback, the event needs to save the data to a volatile.
    // daynight_state = data;
    return;
}

typedef void (*PointerToCallback)(uint8_t data);

static PointerToCallback twi0_onDayNightState = twi0_callback_default;
static PointerToCallback twi0_onDayWork = twi0_callback_default;
static PointerToCallback twi0_onNightWork = twi0_callback_default;

volatile static int a;

// I2C command returning the managers daynight_state
void fnDayNightState(uint8_t* i2cBuffer)
{
    uint8_t data = i2cBuffer[1];
    twi0_onDayNightState(data);
}

// I2C command returning the day work event
void fnDayWork(uint8_t* i2cBuffer)
{
    uint8_t data = i2cBuffer[1];
    twi0_onDayWork(data);
}

// I2C command returning the night work event
void fnNightWork(uint8_t* i2cBuffer)
{
    uint8_t data = i2cBuffer[1];
    twi0_onNightWork(data);
}

/* Dummy function */
void fnNull(uint8_t* i2cBuffer)
{
    return; 
}

// record callback to use durring a DayNightState operation 
// a NULL pointer will use the default callback
void twi0_registerOnDayNightStateCallback( void (*function)(uint8_t data) )
{
    if (function == ((void *)0) )
    {
        twi0_onDayNightState = twi0_callback_default;
    }
    else
    {
        twi0_onDayNightState = function;
    }
}

// record callback to use durring a DayNightState operation 
// a NULL pointer will use the default callback
void twi0_registerOnDayWorkCallback( void (*function)(uint8_t data) )
{
    if (function == ((void *)0) )
    {
        twi0_onDayWork = twi0_callback_default;
    }
    else
    {
        twi0_onDayWork = function;
    }
}

// record callback to use durring a DayNightState operation 
// a NULL pointer will use the default callback
void twi0_registerOnNightWorkCallback( void (*function)(uint8_t data) )
{
    if (function == ((void *)0) )
    {
        twi0_onNightWork = twi0_callback_default;
    }
    else
    {
        twi0_onNightWork = function;
    }
}
