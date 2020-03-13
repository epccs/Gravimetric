/*
i2c-monitor is a library that monitors an i2c bus slave address
Copyright (C) 2019 Ronald Sutherland

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

#include <stdio.h>
#include <stdlib.h> 
#include <stdbool.h>
#include <avr/pgmspace.h>
#include "../lib/uart0_bsd.h"
#include "../lib/parse.h"
#include "../lib/twi1_bsd.h"
#include "i2c1-monitor.h"

static uint8_t slave_addr;

static uint8_t localBuffer[TWI1_BUFFER_LENGTH];
static uint8_t localBufferLength;

static uint8_t printBuffer[TWI1_BUFFER_LENGTH];
static uint8_t printBufferLength;
static uint8_t printBufferIndex;

// echo what was received
void twi1_transmit_callback(void)
{
    /*uint8_t return_code = */ twi1_fillSlaveTxBuffer(localBuffer, localBufferLength);
    
    /* todo: add status_byt to main so I can use it to blink the LED fast or for reporting
    if (return_code != 0)
        status_byt &= (1<<DTR_I2C_TRANSMIT_FAIL);
    */
    return;
}

// Place the received data in local buffer so it can echo back.
// If monitor is running, printing done, and UART is available 
// fill the print buffer and reset the index for printing
void twi1_receive_callback(uint8_t *data, uint8_t length)
{
    localBufferLength = length;
    for(int i = 0; i < length; ++i)
    {
        localBuffer[i] = data[i];
    }
    if (slave_addr && (printBufferLength == printBufferIndex) && uart0_availableForWrite())
    {
        printBufferLength = length;
        printBufferIndex = 0;
        for(int i = 0; i < length; ++i)
        {
            printBuffer[i] = data[i];
        }
    }
    return;
}

/****** PUBLIC **************/

// Monitor a I2C slave address.
void I2c1_monitor(void)
{
    if (command_done == 10)
    {
        // check that argument[0] is 7 bits e.g. the range (0x8..0x77)
        slave_addr = is_arg_in_uint8_range(0,0x8,0x77);
        if ( !slave_addr )
        {
            printf_P(PSTR("{\"bad_slave_addr\":\"0x%X\"}\r\n"),slave_addr);
            initCommandBuffer();
            return;
        }

        twi1_registerSlaveRxCallback(twi1_receive_callback);
        twi1_registerSlaveTxCallback(twi1_transmit_callback);
        twi1_slaveAddress(slave_addr); // ISR is enabled so register callback first

        command_done = 11;
    }

    else if (command_done == 11)
    {
        if (printBufferIndex < printBufferLength)
        {
            if (printBufferIndex > 0)
            {
                printf_P(PSTR(","));
            }
            else
            {
                printf_P(PSTR("{\"monitor_0x%X\":["),slave_addr); // start of JSON for monitor
            }
            printf_P(PSTR("{\"data\":\"0x%X\"}"),printBuffer[printBufferIndex]);
            printBufferIndex += 1;
            if (printBufferIndex >= printBufferLength) 
            {
                command_done = 12; // done printing 
            }
        }
        else
        {
            return; // slave receive did not happen yet
        }
    }

    else if ( (command_done == 12) )
    {
        printf_P(PSTR("]"));
        command_done = 13;
    }
    
    else if ( (command_done == 13) )
    {
        printf_P(PSTR("}\r\n"));
        command_done = 11; // wait for next slave receive event to fill printBuffer
    }

    else
    {
        initCommandBuffer();
    }
}

