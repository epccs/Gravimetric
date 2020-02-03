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

Note the library files are LGPL, e.g., you need to publish changes of them but can derive from this 
source and copyright or distribute as you see fit (it is Zero Clause BSD).

https://en.wikipedia.org/wiki/BSD_licenses#0-clause_license_(%22Zero_Clause_BSD%22)
*/
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h> 
#include "../lib/parse.h"
#include "../lib/twi0.h"
#include "i2c0-scan.h"

static uint8_t address;
static uint8_t start_address;
static uint8_t end_address;
static uint8_t found_addresses;
static uint8_t returnCode;

/* Scan the I2C bus between addresses from_addr and to_addr.
   On each address, call the callback function with the address and result.
   If result==0, address was found, otherwise, address wasn't found
   can use result to potentially get other status off the I2C bus, see twi.c   */
void I2c0_scan(void)
{
    if (command_done == 10)
    {
        start_address = 0x8; // 0-0x7 is reserved (e.g. general call, CBUS, Hs-mode...)
        end_address = 0x77; // 0x78-0x7F is reserved (e.g. 10-bit...)
        address = start_address;
        found_addresses = 0;
        printf_P(PSTR("{\"scan\":[")); // start of JSON
        command_done = 11;
    }
    
    else if (command_done == 11)
    {
        uint8_t data = 0;
        uint8_t length = 0;
        uint8_t wait = 1;
        uint8_t sendStop = 1;
        returnCode = twi0_writeTo(address, &data, length, wait, sendStop); 
        
        if ( (returnCode == 0) && (found_addresses > 0) )
        {
            printf_P(PSTR(","));
        }
        
        if (returnCode == 0)
        {
            printf_P(PSTR("{\"addr\":\"0x%X\"}"),address);
            found_addresses += 1;
        }
        
        if (address >= end_address) 
        {
            command_done = 12;
        }
        else
        {
            address += 1;
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

