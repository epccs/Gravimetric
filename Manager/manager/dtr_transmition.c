/*
DTR pair transmition for multidrop serial (RPUBUS) manager(s)
Copyright (C) 2019 Ronald Sutherland

All rights reserved, specifically, the right to Redistribut is withheld. Subject 
to your compliance with these terms, you may use this software and derivatives. 

Use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:
1. Source code must retain the above copyright notice, this list of 
conditions and the following disclaimer.
2. Binary derivatives are exclusively for use with Ronald Sutherland 
products.
3. Neither the name of the copyright holders nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

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
*/

#include <stdbool.h>
#include <util/delay.h>
#include <avr/io.h>
#include "../lib/timers_bsd.h"
#include "../lib/uart0_bsd.h"
#include "../lib/io_enum_bsd.h"
#include "main.h"
#include "rpubus_manager_state.h"
#include "daynight_state.h"
#include "battery_manager.h"
#include "host_shutdown_manager.h"
#include "dtr_transmition.h"


// public
unsigned long uart_started_at;

volatile uint8_t uart_output;


// private
unsigned long target_reset_started_at;

uint8_t uart_previous_byte;
uint8_t my_mcu_is_target_and_i_have_it_reset;

void check_DTR(void)
{
    if (!host_is_foreign) 
    {
        if ( !ioRead(MCU_IO_HOST_nRTS) )  // if HOST_nRTS is set (active low) then assume avrdude wants to use the bootloader
        {
            if ( !(status_byt & (1<<HOST_LOCKOUT_STATUS)) )
            {
                if (ioRead(MCU_IO_HOST_nCTS))
                { // tell the host that it is OK to use serial
                    ioWrite(MCU_IO_HOST_nCTS, LOGIC_LEVEL_LOW);
                }
                else
                {
                    if ( !(bootloader_started  || lockout_active || host_active || uart_has_TTL) )
                    {
                        // send the bootload_addres on the DTR pair when nDTR/nRTS becomes active
                        uart_started_at = milliseconds();
                        uart_output= bootloader_address; // set by I2C, default is RPU_HOST_CONNECT
                        printf("%c%c", uart_output, ( (~uart_output & 0x0A) << 4 | (~uart_output & 0x50) >> 4 )  ); 
                        uart_has_TTL = 1;
                        localhost_active = 1;
                    }
                }
            }
        }
        else
        {
            if ( host_active && localhost_active && (!uart_has_TTL) && (!bootloader_started) && (!lockout_active) )
            {
                // send a byte on the DTR pair when local host serial nRTS becomes non-active
                uart_started_at = milliseconds();
                uart_output= RPU_HOST_DISCONNECT;
                printf("%c%c", uart_output, ( (~uart_output & 0x0A) << 4 | (~uart_output & 0x50) >> 4 ) ); 
                uart_has_TTL = 1;
                ioWrite(MCU_IO_MGR_SCK_LED, LOGIC_LEVEL_HIGH);
                localhost_active = 0;
                ioWrite(MCU_IO_HOST_nCTS, LOGIC_LEVEL_HIGH);
            }
        }
    }
}

void connect_bootload_mode(void)
{
    // connect the remote host and local mcu
    if (host_is_foreign)
    {
        ioWrite(MCU_IO_RX_DE, LOGIC_LEVEL_LOW); // disallow RX pair driver to enable if FTDI_TX is low
        ioWrite(MCU_IO_RX_nRE, LOGIC_LEVEL_LOW);  // enable RX pair recevior to output to local MCU's RX input
        ioWrite(MCU_IO_TX_DE, LOGIC_LEVEL_HIGH); // allow TX pair driver to enable if TX (from MCU) is low
        ioWrite(MCU_IO_TX_nRE, LOGIC_LEVEL_HIGH);  // disable TX pair recevior to output to FTDI_RX input
    }
    
    // connect the local host and local mcu
    else
    {
        ioWrite(MCU_IO_RX_DE, LOGIC_LEVEL_HIGH); // allow RX pair driver to enable if FTDI_TX is low
        ioWrite(MCU_IO_RX_nRE, LOGIC_LEVEL_LOW);  // enable RX pair recevior to output to local MCU's RX input
        ioWrite(MCU_IO_TX_DE, LOGIC_LEVEL_HIGH); // allow TX pair driver to enable if TX (from MCU) is low
        ioWrite(MCU_IO_TX_nRE, LOGIC_LEVEL_LOW);  // enable TX pair recevior to output to FTDI_RX input
    }
}

void connect_lockout_mode(void)
{
    // lockout everything
    if (host_is_foreign)
    {
        ioWrite(MCU_IO_RX_DE, LOGIC_LEVEL_LOW); // disallow RX pair driver to enable if FTDI_TX is low
        ioWrite(MCU_IO_RX_nRE, LOGIC_LEVEL_HIGH);  // disable RX pair recevior to output to local MCU's RX input
        ioWrite(MCU_IO_TX_DE, LOGIC_LEVEL_LOW); // disallow TX pair driver to enable if TX (from MCU) is low
        ioWrite(MCU_IO_TX_nRE, LOGIC_LEVEL_HIGH);  // disable TX pair recevior to output to FTDI_RX input
    }
    
    // lockout MCU, but not host
    else
    {
        ioWrite(MCU_IO_RX_DE, LOGIC_LEVEL_HIGH); // allow RX pair driver to enable if FTDI_TX is low
        ioWrite(MCU_IO_RX_nRE, LOGIC_LEVEL_HIGH);  // disable RX pair recevior to output to local MCU's RX input
        ioWrite(MCU_IO_TX_DE, LOGIC_LEVEL_LOW); // disallow TX pair driver to enable if TX (from MCU) is low
        ioWrite(MCU_IO_TX_nRE, LOGIC_LEVEL_LOW);  // enable TX pair recevior to output to FTDI_RX input
    }
}

/* The UART is connected to the DTR pair which is half duplex, 
     but is self enabling when TX is pulled low.

     Both I2C events and nRTS events (e.g., check_DTR) place state changes on 
     the DTR pair. This function drives those changes.
     ToDo: a state machine with enums: RPU_NORMAL_MODE, RPU_ARDUINO_MODE, RPU_START_TEST_MODE, RPU_END_TEST_MODE...
*/
void check_uart(void)
{
    unsigned long kRuntime = elapsed(&uart_started_at);
 
    if ( uart_has_TTL && (kRuntime > UART_TTL) )
    { // perhaps the DTR line is stuck (e.g. pulled low) so may need to time out
        status_byt &= (1<<DTR_READBACK_TIMEOUT);
        uart_has_TTL = 0;
    }
    else
    {
        if ( uart0_available() )
        {
            uint8_t input;
            input = (uint8_t)(getchar());
            
            // The test interface can glitch the DTR pair, so a check byte is used to make 
            // sure the data is real and not caused by testing.
            // how the check byte was made:   ( (~uart_output & 0x0A) << 4 | (~uart_output & 0x50) >> 4 ) 
            // do that to the previous input to see if this input is a valid check.
            if (  (input ==  ( (~uart_previous_byte & 0x0A) << 4 | (~uart_previous_byte & 0x50) >> 4 ) )  )
            {
                input = uart_previous_byte; // replace input with the valid byte. 
                uart_previous_byte = 0; 
            }
            else
            {
                uart_previous_byte = input; // this byte may be a state change or a glitch
                return;
            }
            

            // was this byte sent with the local DTR pair driver, if so the status_byt may need update
            // and the lockout from a local host needs to be treated differently since I 
            // need to ignore the local host's nRTS if getting control from a remote host
            if ( uart_has_TTL )
            {
                if(input != uart_output) 
                { // sent byte did not match.
                    status_byt &= (1<<DTR_READBACK_NOT_MATCH);
                }
                uart_has_TTL = 0;
                host_is_foreign = 0;
            }
            else
            {
                if (localhost_active)
                {
                    host_is_foreign = 0; // used to connect the host
                }
                else
                {
                    host_is_foreign = 1; // used to lockout the host
                }
            }

            if (input == RPU_NORMAL_MODE) // end the lockout or bootloader if it was set.
            { 
                lockout_started_at = milliseconds() - LOCKOUT_DELAY;
                bootloader_started_at = milliseconds() - BOOTLOADER_ACTIVE;
                ioWrite(MCU_IO_MGR_SCK_LED, LOGIC_LEVEL_LOW);
                arduino_mode = 0;
                blink_started_at = milliseconds();
                return;
            }
            if (input == RPU_ARDUINO_MODE) 
            {
                arduino_mode_started = 0;
                arduino_mode = 1;
                return;
            }
            if (input == RPU_START_TEST_MODE) 
            {
                // fill transceiver_state with HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE
                transceiver_state = (ioRead(MCU_IO_HOST_nRTS)<<7) | (ioRead(MCU_IO_HOST_nCTS)<<6) |  (ioRead(MCU_IO_TX_nRE)<<5) | \
                                    (ioRead(MCU_IO_TX_DE)<<4) | (ioRead(MCU_IO_DTR_nRE)<<3) | (ioRead(MCU_IO_DTR_DE)<<2) | \
                                    (ioRead(MCU_IO_RX_nRE)<<1) | (ioRead(MCU_IO_RX_DE));
                // turn off batter manager (which controls ALT_EN) with command 16; set the callback address to zero
                // ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);
                // turn off transceiver controls except the DTR recevior
                ioWrite(MCU_IO_TX_nRE, LOGIC_LEVEL_HIGH);
                ioWrite(MCU_IO_TX_DE, LOGIC_LEVEL_LOW);
                // DTR_nRE active would block uart from seeing RPU_END_TEST_MODE
                ioWrite(MCU_IO_DTR_DE, LOGIC_LEVEL_LOW); 
                ioWrite(MCU_IO_RX_nRE, LOGIC_LEVEL_HIGH);
                ioWrite(MCU_IO_RX_DE, LOGIC_LEVEL_LOW);

                test_mode_started = 0;
                test_mode = 1;
                return;
            }
            if (input == RPU_END_TEST_MODE) 
            {
                // recover transceiver controls
                ioWrite(MCU_IO_HOST_nRTS, ( (transceiver_state>>7) & 0x01) );
                ioWrite(MCU_IO_HOST_nCTS, ( (transceiver_state>>6) & 0x01) );
                ioWrite(MCU_IO_TX_nRE, ( (transceiver_state>>5) & 0x01) );
                ioWrite(MCU_IO_TX_DE, ( (transceiver_state>>4) & 0x01) );
                // DTR_nRE is always active... but
                ioWrite(MCU_IO_DTR_nRE, ( (transceiver_state>>3) & 0x01) );
                // the I2C command fnEndTestMode() sets the DTR_TXD pin and turns on the UART... but
                ioWrite(MCU_IO_DTR_TXD,LOGIC_LEVEL_HIGH); // strong pullup
                ioDir(MCU_IO_DTR_TXD, DIRECTION_INPUT); // the DTR pair driver will see a weak pullup when UART starts
                UCSR0B |= (1<<RXEN0)|(1<<TXEN0); // turn on UART
                ioWrite(MCU_IO_DTR_DE, ( (transceiver_state>>2) & 0x01) );
                ioWrite(MCU_IO_RX_nRE, ( (transceiver_state>>1) & 0x01) );
                ioWrite(MCU_IO_RX_DE, ( (transceiver_state) & 0x01) );

                test_mode_started = 0;
                test_mode = 0;
                return;
            }
            if (input == rpu_address) // that is my local address
            {
                if(!my_mcu_is_target_and_i_have_it_reset)
                {
                    connect_bootload_mode();

                    // start the bootloader
                    ioWrite(MCU_IO_MGR_nSS, LOGIC_LEVEL_LOW);   // nSS goes through a open collector buffer to nRESET
                    bm_callback_address = 0;
                    bm_callback_route = 0; // turn off i2c battery manager callback
                    shutdown_callback_address = 0;
                    shutdown_callback_route = 0; // turn off i2c host shutdown callback
                    daynight_callback_address = 0;
                    daynight_callback_route = 0;
                    target_reset_started_at = milliseconds();
                    my_mcu_is_target_and_i_have_it_reset = 1;
                    return; 
                }
                unsigned long kRuntime = elapsed(&target_reset_started_at);
                if (kRuntime < 20UL) // hold reset low for a short time but don't delay (the mcu runs 200k instruction in 20 mSec)
                {
                    return;
                } 
                //_delay_ms(20);  // hold reset low for a short time, but this locks the mcu which which blocks i2c, SMBus, and ADC burst. 
                ioWrite(MCU_IO_MGR_nSS, LOGIC_LEVEL_HIGH); // this will release the buffer with open colllector on MCU nRESET.
                my_mcu_is_target_and_i_have_it_reset = 0;
                bootloader_started = 1;
                local_mcu_is_rpu_aware = 0; // after a reset it may be loaded with new software
                blink_started_at = milliseconds();
                bootloader_started_at = milliseconds();
                return;
            }
            if (input <= 0x7F) // values > 0x80 are for a host disconnect e.g. the bitwise negation of an RPU_ADDRESS
            {  
                lockout_active =1;
                bootloader_started = 0;
                host_active =0;

                connect_lockout_mode();

                lockout_started_at = milliseconds();
                blink_started_at = milliseconds();
                return;
            }
            if (input > 0x7F) // RPU_HOST_DISCONNECT is the bitwise negation of an RPU_ADDRESS it will be > 0x80 (seen as a uint8_t)
            { 
                host_is_foreign = 0;
                lockout_active =0;
                host_active =0;
                bootloader_started = 0;
                ioWrite(MCU_IO_MGR_SCK_LED, LOGIC_LEVEL_HIGH);
                ioWrite(MCU_IO_RX_DE, LOGIC_LEVEL_LOW); // disallow RX pair driver to enable if FTDI_TX is low
                ioWrite(MCU_IO_RX_nRE, LOGIC_LEVEL_HIGH);  // disable RX pair recevior to output to local MCU's RX input
                ioWrite(MCU_IO_TX_DE, LOGIC_LEVEL_LOW); // disallow TX pair driver to enable if TX (from MCU) is low
                ioWrite(MCU_IO_TX_nRE, LOGIC_LEVEL_HIGH);  // disable TX pair recevior that outputs to FTDI_RX input
                return;
            }
            // nothing can get past this point.
            return;
        }
    }
}