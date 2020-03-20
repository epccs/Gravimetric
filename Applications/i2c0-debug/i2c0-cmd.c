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

https://en.wikipedia.org/wiki/BSD_licenses#0-clause_license_(%22Zero_Clause_BSD%22)
*/
#include <stdio.h>
#include <stdlib.h> 
#include <ctype.h>
#include <avr/pgmspace.h>
#include "../lib/parse.h"
#include "../lib/twi0_bsd.h"
#include "i2c0-cmd.h"

static uint8_t master_address = 0; // I2C addresses master will access

static uint8_t txBufferPassedToMaster[TWI0_BUFFER_LENGTH];   // transmit buffer that will be passed to I2C master
static uint8_t txBufferPassedToMaster_index = 0;

static uint8_t rxBufferAcceptFromMaster[TWI0_BUFFER_LENGTH]; // receive buffer that will be accepted from I2C master
static uint8_t rxBufferAcceptFromMaster_lenght = 0;

static uint8_t JsonIndex;


// set I2C bus addresses master will access and clear the txBufferPassedToMaster
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

        txBufferPassedToMaster_index = 0;
        master_address = arg0; 
        printf_P(PSTR("{\"master_address\":\"0x%X\"}\r\n"),master_address);
        initCommandBuffer();
    }

    else
    {
        initCommandBuffer();
    }
}


// buffer bytes into txBufferPassedToMaster
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
            if ( txBufferPassedToMaster_index >= TWI0_BUFFER_LENGTH)
            {
                printf_P(PSTR("{\"err\":\"I2CBufOVF\"}\r\n"));
                txBufferPassedToMaster_index = 0;
                initCommandBuffer();
                return;
            }
            txBufferPassedToMaster[txBufferPassedToMaster_index] = (uint8_t) atoi(arg[arg_index]);
            txBufferPassedToMaster_index += 1; 
        }

        printf_P(PSTR("{\"txBuffer[%d]\":["),txBufferPassedToMaster_index);
        JsonIndex = 0;
        command_done = 11;
    }
    
    else if (command_done == 11)
    {
        // txBufferPassedToMaster_index is pointing to the next position data would be placed
        if ( JsonIndex >= txBufferPassedToMaster_index ) 
        {
            command_done = 12;
        }
        else
        {
            if (JsonIndex > 0)
            {
                printf_P(PSTR(","));
            }
            printf_P(PSTR("{\"data\":\"0x%X\"}"),txBufferPassedToMaster[JsonIndex]);
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

/* write the I2C txBufferPassedToMaster */
void I2c0_write(void)
{
    if (command_done == 10)
    {
        uint8_t txBufferLength = txBufferPassedToMaster_index;
        TWI0_WRT_t twi_attempt = twi0_masterAsyncWrite(master_address, txBufferPassedToMaster, txBufferLength, TWI0_PROTOCALL_STOP); 
        if (twi_attempt == TWI0_WRT_TRANSACTION_STARTED)
        {
            command_done = 11;
            return; // back to loop
        }
        else if(twi_attempt == TWI0_WRT_TO_MUCH_DATA)
        {
            printf_P(PSTR("{\"error\":\"wrt_data_to_much\""));
            command_done = 20;
            return;
        }
        else if(twi_attempt == TWI0_WRT_NOT_READY)
        {
            // could use a timer to print a . every sec
            return; // i2c is not ready so back to loop
        }
    }

    else if (command_done == 11)
    {
        TWI0_WRT_STAT_t status = twi0_masterAsyncWrite_status(); 
        if (status == TWI0_WRT_STAT_BUSY)
        {
            return; // twi write operation not complete, so back to loop
        }

        // address send, NACK received
        if (status == TWI0_WRT_STAT_ADDR_NACK)
        {
            printf_P(PSTR("{\"error\":\"wrt_addr_nack\""));
        }

        // data send, NACK received
        if (status == TWI0_WRT_STAT_DATA_NACK)
        {
            printf_P(PSTR("{\"error\":\"wrt_data_nack\""));
        }

        // illegal start or stop condition
        if (status == TWI0_WRT_STAT_ILLEGAL)
        {
            // is a master trying to take the bus?
            printf_P(PSTR("{\"error\":\"wrt_illegal\""));
        }

        if ( (status == TWI0_WRT_STAT_SUCCESS) )
        {
            printf_P(PSTR("{\"txBuffer\":\"wrt_success\""));
            txBufferPassedToMaster_index = 0;
        }
        command_done = 20;
    }
    
    else if (command_done == 20)
    {
        printf_P(PSTR("}\r\n"));
        initCommandBuffer();
    }
    
    else
    {
        initCommandBuffer();
    }
}


// write the byte(s) in txBufferPassedToMaster (e.g., command byte) to I2C address 
// without a stop condition. Then have the ISR set the Start condition (e.g., a repeated Start)
// and then do an I2C read operation. 
// If txBufferPassedToMaster is empty skip the write. 
void I2c0_read(void)
{
    if (command_done == 10)
    {
        // check that argument[0] range 1..32
        if ( ( !( isdigit(arg[0][0]) ) ) || (atoi(arg[0]) < 1) || (atoi(arg[0]) > TWI0_BUFFER_LENGTH) )
        {
            printf_P(PSTR("{\"err\":\"BufferMax %d\"}\r\n"),TWI0_BUFFER_LENGTH);
            initCommandBuffer();
            return;
        }
        
        // send command byte(s) without a Stop (causing a repeated Start durring read)
        TWI0_WRT_t twi_attempt;
        if (txBufferPassedToMaster_index) 
        {
            uint8_t txBufferLength = txBufferPassedToMaster_index;
            twi_attempt = twi0_masterAsyncWrite(master_address, txBufferPassedToMaster, txBufferLength, TWI0_PROTOCALL_REPEATEDSTART);
        }
        else
        {
            printf_P(PSTR("{"));
            command_done = 20;
            return; // back to loop
        }
        if (twi_attempt == TWI0_WRT_TRANSACTION_STARTED)
        {
            command_done = 11;
            return; // back to loop
        }
        else if(twi_attempt == TWI0_WRT_TO_MUCH_DATA)
        {
            printf_P(PSTR("{\"error\":\"wrt_data_to_much\""));
            command_done = 30;
        }
        else if(twi_attempt == TWI0_WRT_NOT_READY)
        {
            // need to add a timeout timer 
            return; // i2c is not ready so back to loop
        }
    }

    else if (command_done == 11)
    {
        TWI0_WRT_STAT_t status = twi0_masterAsyncWrite_status(); 
        if (status == TWI0_WRT_STAT_BUSY)
        {
            return; // twi write operation not complete, so back to loop
        }

        // address send, NACK received
        if (status == TWI0_WRT_STAT_ADDR_NACK)
        {
            printf_P(PSTR("{\"error\":\"wrt_addr_nack\""));
            command_done = 30;
        }

        // data send, NACK received
        if (status == TWI0_WRT_STAT_DATA_NACK)
        {
            printf_P(PSTR("{\"error\":\"wrt_data_nack\""));
            command_done = 30;
        }

        // illegal start or stop condition
        if (status == TWI0_WRT_STAT_ILLEGAL)
        {
            // is a master trying to take the bus?
            printf_P(PSTR("{\"error\":\"wrt_illegal\""));
            command_done = 30;
        }

        if ( (status == TWI0_WRT_STAT_SUCCESS) )
        {
            printf_P(PSTR("{\"txBuffer\":\"wrt_success\","));
            txBufferPassedToMaster_index = 0;
            command_done = 20;
        }
    }

    else if (command_done == 20)
    {
        uint8_t quantity = (uint8_t) atoi(arg[0]); // arg[0] has been checked to be in range 1..32
        TWI0_RD_t twi_attempt = twi0_masterAsyncRead(master_address, quantity, TWI0_PROTOCALL_STOP);
        if (twi_attempt == TWI0_RD_TRANSACTION_STARTED)
        {
            command_done = 21;
            return; // back to loop
        }
        else if(twi_attempt == TWI0_RD_TO_MUCH_DATA)
        {
            printf_P(PSTR("\"error\":\"rd_data_to_much\""));
            command_done = 30;
        }
        else if(twi_attempt == TWI0_RD_NOT_READY)
        {
            // need to add a timeout timer 
            return; // i2c is not ready so back to loop
        }
    }

    else if (command_done == 21)
    {
        TWI0_RD_STAT_t status = twi0_masterAsyncRead_status(); 
        if (status == TWI0_RD_STAT_BUSY)
        {
            return; // twi read operation not complete, so back to loop
        }

        // address send, NACK received
        if (status == TWI0_RD_STAT_ADDR_NACK)
        {
            printf_P(PSTR("\"error\":\"rd_addr_nack\""));
            command_done = 30;
        }

        // data receive, NACK sent (to soon)
        if (status == TWI0_RD_STAT_DATA_NACK)
        {
            printf_P(PSTR("\"error\":\"rd_data_nack\""));
            command_done = 30;
        }

        // illegal start or stop condition
        if (status == TWI0_RD_STAT_ILLEGAL)
        {
            // is a master trying to take the bus?
            printf_P(PSTR("\"error\":\"rd_illegal\""));
            command_done = 30;
        }

        if ( (status == TWI0_RD_STAT_SUCCESS) )
        {
            printf_P(PSTR("\"rxBuffer\":\"rd_success\","));
            command_done = 22;
        }
    }

    else if (command_done == 22)
    {
        rxBufferAcceptFromMaster_lenght = twi0_masterAsyncRead_getBytes(rxBufferAcceptFromMaster); 
        printf_P(PSTR("\"rxBuffer\":["));
        JsonIndex = 0;
        command_done = 23;
    }

    else if (command_done == 23)
    {
        if (JsonIndex > 0)
        {
            printf_P(PSTR(","));
        }
        printf_P(PSTR("{\"data\":\"0x%X\"}"),rxBufferAcceptFromMaster[JsonIndex]);

        if ( JsonIndex >= (rxBufferAcceptFromMaster_lenght-1) ) 
        {
            printf_P(PSTR("]"));
            command_done = 30;
        }
        else
        {
            JsonIndex += 1;
        }
    }

    else if (command_done == 30)
    {
        printf_P(PSTR("}\r\n"));
        initCommandBuffer();
    }

    else
    {
        initCommandBuffer();
    }
}