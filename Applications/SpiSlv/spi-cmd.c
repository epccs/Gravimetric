/*
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
#include <util/atomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include "../lib/parse.h"
#include "../lib/rpu_mgr.h"
#include "../lib/io_enum_bsd.h"
#include "spi-cmd.h"

volatile uint8_t spi_data;

// SPDR is used to shift data in and out, so it will echo back on SPI what was last sent.
// A copy is made for local use (i.e. SPDR will shift when the master drives SCK)
ISR(SPI_STC_vect)
{
    // The system is single buffered in the transmit direction and 
    // double buffered in the receive direction.
    spi_data = SPDR;
}

// SPI slave setup, R-Pi is the master
void spi_init(void)
{
    // SPI slave setup 
    ioDir(MCU_IO_ICP3_MOSI, DIRECTION_INPUT);
    ioWrite(MCU_IO_ICP3_MOSI, LOGIC_LEVEL_HIGH); // weak pull up 
    ioDir(MCU_IO_MISO, DIRECTION_OUTPUT);
    ioDir(MCU_IO_SCK, DIRECTION_INPUT); // an SPI slave is cloced by the master
    ioWrite(MCU_IO_SCK, LOGIC_LEVEL_HIGH); // weak pull up 
    ioDir(MCU_IO_nSS, DIRECTION_INPUT); // connected to nCE00 from Raspberry Pi
    ioWrite(MCU_IO_nSS, LOGIC_LEVEL_HIGH); // weak pull up 
}

void EnableSpi(void)
{
    if ( (command_done == 10) )
    {
        // check arg[0] is not ('UP' or 'DOWN')
        if ( !( (strcmp_P( arg[0], PSTR("UP")) == 0) || (strcmp_P( arg[0], PSTR("DOWN")) == 0) ) ) 
        {
            printf_P(PSTR("{\"err\":\"SpiNaMode\"}\r\n"));
            initCommandBuffer();
            return;
        }
        if (strcmp_P( arg[0], PSTR("UP")) == 0 ) 
        {
            // DORD bit zero transmits MSB of SPDR first
            // MSTR bit zero slave SPI mode
            // CPOL bit zero idle while SCK is low (active high)
            // CPHA bit zero sample data on active going SCK edge and setup on the non-active going edge
            SPCR = (1<<SPE)|(1<<SPIE);       // SPI Enable and SPI interrupt enable bit
            SPDR = 0;                        // SPI data register used for shifting data
            printf_P(PSTR("{\"SPI\":\"UP\"}\r\n"));
            initCommandBuffer();
        }
        else
        {
            SPCR = 0;                        // SPI Disanable SPI
            SPDR = 0;                        // SPI data register used for shifting data
            printf_P(PSTR("{\"SPI\":\"DOWN\"}\r\n"));
            initCommandBuffer();
        }
    }
    else
    {
        printf_P(PSTR("{\"err\":\"SpiCmdDnWTF\"}\r\n"));
        initCommandBuffer();
    }
}

