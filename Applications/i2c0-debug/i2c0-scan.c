/*
i2c-scan is a library that scans the i2c bus
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
#include <avr/pgmspace.h>
#include "../lib/parse.h"
#include "../lib/twi0_bsd.h"
#include "i2c0-scan.h"

static uint8_t address;
static uint8_t start_address;
static uint8_t end_address;
static uint8_t found_addresses;

// Scan the I2C bus between addresses from_addr and to_addr.
void I2c0_scan(void)
{
    if (command_done == 10)
    {
        start_address = 0x8; // 0-0x7 is reserved (e.g. general call, CBUS, Hs-mode...)
        end_address = 0x77; // 0x78-0x7F is reserved (e.g. 10-bit...)
        address = start_address;
        found_addresses = 0; // count of addresses found, it is used to format JSON
        printf_P(PSTR("{\"scan\":[")); // start of JSON
        command_done = 11;
    }

    else if (command_done == 11)
    {
        uint8_t data = 0;
        uint8_t length = 0;
        // attempt TWI asynchronous write.
        TWI0_WRT_t twi_attempt = twi0_masterAsyncWrite(address, &data, length, TWI0_PROTOCALL_STOP); 
        if (twi_attempt == TWI0_WRT_TRANSACTION_STARTED)
        {
            command_done = 12;
            return; // back to loop
        }
        else if(twi_attempt == TWI0_WRT_TO_MUCH_DATA)
        {
            if (found_addresses > 0)
            {
                printf_P(PSTR(","));
            }
            printf_P(PSTR("{\"error\":\"data_to_much\"}"));
            command_done = 13;
            return;
        }
        else // if(twi_attempt == TWI0_WRT_NOT_READY)
        {
            return; // twi is not ready so back to loop
        }
    }

    else if (command_done == 12)
    {
        TWI0_WRT_STAT_t status;
        status = twi0_masterAsyncWrite_status(); 
        if (status == TWI0_WRT_STAT_BUSY)
        {
            return; // twi write operation not complete, so back to loop
        }

        // address send, NACK received
        if (status == TWI0_WRT_STAT_ADDR_NACK)
        {
            /* do not report
            if (found_addresses > 0)
            {
                printf_P(PSTR(","));
            }
            printf_P(PSTR("{\"error\":\"addr_nack\""));
            found_addresses += 1;
            */
        }

        // data send, NACK received
        if (status == TWI0_WRT_STAT_DATA_NACK)
        {
            // should never report
            if (found_addresses > 0)
            {
                printf_P(PSTR(","));
            }
            printf_P(PSTR("{\"error\":\"data_nack\""));
            found_addresses += 1;
        }

        // illegal start or stop condition
        if (status == TWI0_WRT_STAT_ILLEGAL)
        {
            // is a master trying to take the bus?
            if (found_addresses > 0)
            {
                printf_P(PSTR(","));
            }
            printf_P(PSTR("{\"error\":\"illegal\"}"));
            found_addresses += 1;
        }

        if ( (status == TWI0_WRT_STAT_SUCCESS) )
        {
            if (found_addresses > 0)
            {
                printf_P(PSTR(","));
            }
            printf_P(PSTR("{\"addr\":\"0x%X\"}"),address);
            found_addresses += 1;
        }
        
        if (address >= end_address) 
        {
            command_done = 13;
        }
        else
        {
            address += 1;
            command_done = 11;
        }
    }
    
    else if ( (command_done == 13) )
    {
        printf_P(PSTR("]}\r\n"));
        initCommandBuffer();
    }

    else
    {
        initCommandBuffer();
    }
}

