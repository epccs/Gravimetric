/*
i2c-cmd is a library that controls the i2c bus as a master
Copyright (C) 2019 Ronald Sutherland

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES 
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE 
FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY 
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, 
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, 
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Note some of the library files are LGPL, e.g., you need to publish changes of them but can derive from this 
source and copyright or distribute as you see fit (it is Zero Clause BSD).

https://en.wikipedia.org/wiki/BSD_licenses#0-clause_license_(%22Zero_Clause_BSD%22)
*/
#include <stdio.h>
#include <stdlib.h> 
#include <ctype.h>
#include <avr/pgmspace.h>
#include "../lib/parse.h"
#include "../lib/twi0_bsd.h"
#include "i2c0-cmd.h"


static uint8_t returnCode;

static uint8_t I2cAddress = 0;

static uint8_t txBuffer[TWI0_BUFFER_LENGTH];
static uint8_t txBufferIndex = 0;

static uint8_t rxBuffer[TWI0_BUFFER_LENGTH];
static uint8_t rxBufferLength = 0;

static uint8_t JsonIndex;


/* set I2C bus addresses and clear the buffer*/
void I2c0_address(void)
{
    if (command_done == 10)
    {
        // check that argument[0] is 7 bits e.g. the range 1..127
        uint8_t arg0 = is_arg_in_uint8_range(0,1,127);
        if ( !arg0 )
        {
            initCommandBuffer();
            return;
        }

        txBufferIndex = 0;
        I2cAddress = arg0; 
        printf_P(PSTR("{\"address\":\"0x%X\"}\r\n"),I2cAddress);
        initCommandBuffer();
    }

    else
    {
        initCommandBuffer();
    }
}


/* buffer bytes into an I2C txBuffer */
void I2c0_txBuffer(void)
{
    
    if (command_done == 10)
    {
        // check that arguments are digit and in the range 0..255
        for (uint8_t arg_index=0; arg_index < arg_count; arg_index++) 
        {
            if ( ( !( isdigit(arg[arg_index][0]) ) ) || (atoi(arg[arg_index]) < 0) || (atoi(arg[arg_index]) > 255) )
            {
                printf_P(PSTR("{\"err\":\"I2CDatOutOfRng\"}\r\n"));
                initCommandBuffer();
                return;
            }
            if ( txBufferIndex >= TWI0_BUFFER_LENGTH)
            {
                printf_P(PSTR("{\"err\":\"I2CBufOVF\"}\r\n"));
                txBufferIndex = 0;
                initCommandBuffer();
                return;
            }
            txBuffer[txBufferIndex] = (uint8_t) atoi(arg[arg_index]);
            txBufferIndex += 1; 
        }

        printf_P(PSTR("{\"txBuffer[%d]\":["),txBufferIndex);
        JsonIndex = 0;
        command_done = 11;
    }
    
    else if (command_done == 11)
    {
        // txBufferIndex is pointing to the next position data would be placed
        if ( JsonIndex >= txBufferIndex ) 
        {
            command_done = 12;
        }
        else
        {
            if (JsonIndex > 0)
            {
                printf_P(PSTR(","));
            }
            printf_P(PSTR("{\"data\":\"0x%X\"}"),txBuffer[JsonIndex]);
            JsonIndex += 1;
        }
    }

    else if ( (command_done == 12) )
    {
        printf_P(PSTR("]}\r\n"));
        initCommandBuffer();
    }

    else
    {
        initCommandBuffer();
    }
}

/* write the I2C txBuffer */
void I2c0_write(void)
{
    if (command_done == 10)
    {
        uint8_t sendStop = 1; 
        uint8_t txBufferLength = txBufferIndex;
        returnCode = twi0_masterBlockingWrite(I2cAddress, txBuffer, txBufferLength, sendStop);
        if (returnCode == 0)
        {
            printf_P(PSTR("{\"returnCode\":\"success\""));
            txBufferIndex = 0; 
        }
        if (returnCode == 1)
            printf_P(PSTR("{\"rtnCode\":\"bufOVF\",\"i2c_addr\":\"%d\""),I2cAddress);
        if ( (returnCode == 2) || (returnCode == 3) )
            printf_P(PSTR("{\"rtnCode\":\"nack\",\"i2c_addr\":\"%d\""),I2cAddress);
        if (returnCode == 4)
            printf_P(PSTR("{\"rtnCode\":\"other\",\"i2c_addr\":\"%d\""),I2cAddress);
        command_done = 11;
    }
    
    else if (command_done == 11)
    {
        printf_P(PSTR("}\r\n"));
        initCommandBuffer();
    }
    
    else
    {
        initCommandBuffer();
    }
}


/* write the byte(s) in buffer (e.g. command byte) to I2C address 
    without a stop condition. Then send a repeated Start condition 
    and address with read bit to obtain readings.*/
void I2c0_read(void)
{
    if (command_done == 10)
    {
        // check that argument[0] range 1..32
        if ( ( !( isdigit(arg[0][0]) ) ) || (atoi(arg[0]) < 1) || (atoi(arg[0]) > TWI0_BUFFER_LENGTH) )
        {
            printf_P(PSTR("{\"err\":\"I2cReadUpTo%d\"}\r\n"),TWI0_BUFFER_LENGTH);
            initCommandBuffer();
            return;
        }
        
        // send command byte(s) without a Stop (causing a repeated Start durring read)
        if (txBufferIndex) 
        {
            uint8_t sendStop = 0; 
            uint8_t txBufferLength = txBufferIndex;
            returnCode = twi0_masterBlockingWrite(I2cAddress, txBuffer, txBufferLength, sendStop);
            if (returnCode == 0)
            {
                printf_P(PSTR("{\"rxBuffer\":["));
                txBufferIndex = 0; 
                command_done = 11;
            }
            else
            {
                if (returnCode == 1)
                    printf_P(PSTR("{\"rtnCode\":\"bufOVF\",\"i2c_addr\":\"%d\""),I2cAddress);
                if ( (returnCode == 2) || (returnCode == 3) )
                    printf_P(PSTR("{\"rtnCode\":\"nack\",\"i2c_addr\":\"%d\""),I2cAddress);
                if (returnCode == 4)
                    printf_P(PSTR("{\"rtnCode\":\"other\",\"i2c_addr\":\"%d\""),I2cAddress);
                printf_P(PSTR("}\r\n"));
                initCommandBuffer();
            }
        }
        else
        {
            printf_P(PSTR("{\"rxBuffer\":["));
            command_done = 11;
        }
    }

    else if (command_done == 11)
    {
        // read I2C, it will cause a repeated start if a command has been sent.
        uint8_t sendStop = 1; 
        uint8_t quantity = (uint8_t) atoi(arg[0]); // arg[0] has been checked to be in range 1..32
        uint8_t read = twi0_masterBlockingRead(I2cAddress, rxBuffer, quantity, sendStop);
        rxBufferLength = read;
        JsonIndex = 0;
        command_done = 12;
    }

    else if (command_done == 12)
    {
        if (JsonIndex > 0)
        {
            printf_P(PSTR(","));
        }
        printf_P(PSTR("{\"data\":\"0x%X\"}"),rxBuffer[JsonIndex]);

        if ( JsonIndex >= (rxBufferLength-1) ) 
        {
            command_done = 13;
        }
        else
        {
            JsonIndex += 1;
        }
    }
    
    else if (command_done == 13)
    {
        printf_P(PSTR("]}\r\n"));
        initCommandBuffer();
    }

    else
    {
        initCommandBuffer();
    }
}