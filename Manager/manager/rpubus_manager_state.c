/*
state for manager of the multidrop serial (RPUBUS)
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
#include "host_shutdown_manager.h"
#include "rpubus_manager_state.h"

unsigned long blink_started_at;
unsigned long lockout_started_at;
unsigned long bootloader_started_at;

uint8_t bootloader_started;
uint8_t host_active;
uint8_t localhost_active;
uint8_t bootloader_address; 
uint8_t lockout_active;
uint8_t uart_has_TTL;
uint8_t host_is_foreign;
uint8_t local_mcu_is_rpu_aware;
uint8_t rpu_address;
uint8_t write_rpu_address_to_eeprom;
uint8_t shutdown_detected;
uint8_t shutdown_started;
uint8_t arduino_mode_started;
uint8_t arduino_mode;
uint8_t test_mode_started;
uint8_t test_mode;
uint8_t transceiver_state;

volatile uint8_t status_byt;

void connect_normal_mode(void)
{
    // connect the local mcu if it has talked to the rpu manager (e.g. got an address)
    if(host_is_foreign)
    {
        ioWrite(MCU_IO_RX_DE, LOGIC_LEVEL_LOW); // disallow RX pair driver to enable if FTDI_TX is low
        ioWrite(MCU_IO_RX_nRE, LOGIC_LEVEL_LOW);  // enable RX pair recevior to output to local MCU's RX input
        if(local_mcu_is_rpu_aware)
        {
            ioWrite(MCU_IO_TX_DE, LOGIC_LEVEL_HIGH); // allow TX pair driver to enable if TX (from MCU) is low
        }
        else
        {
            ioWrite(MCU_IO_TX_DE, LOGIC_LEVEL_LOW); // disallow TX pair driver to enable if TX (from MCU) is low
        }
        ioWrite(MCU_IO_TX_nRE, LOGIC_LEVEL_HIGH);  // disable TX pair recevior to output to FTDI_RX input
    }

     // connect both the local mcu and host/ftdi uart if mcu is rpu aware, otherwise block MCU from using the TX pair
    else
    {
        ioWrite(MCU_IO_RX_DE, LOGIC_LEVEL_HIGH); // allow RX pair driver to enable if FTDI_TX is low
        ioWrite(MCU_IO_RX_nRE, LOGIC_LEVEL_LOW);  // enable RX pair recevior to output to local MCU's RX input
        if(local_mcu_is_rpu_aware)
        {
            ioWrite(MCU_IO_TX_DE, LOGIC_LEVEL_HIGH); // allow TX pair driver to enable if TX (from MCU) is low
        }
        else
        {
            ioWrite(MCU_IO_TX_DE, LOGIC_LEVEL_LOW); // disallow TX pair driver to enable if TX (from MCU) is low
        }
        ioWrite(MCU_IO_TX_nRE, LOGIC_LEVEL_LOW);  // enable TX pair recevior to output to FTDI_RX input
    }
}

// blink if the host is active, fast blink if status_byt, slow blink in lockout
void blink_on_activate(void)
{
    // do not blink when host is being shutdown, e.g. states between up and down
    if ( (shutdown_state > HOSTSHUTDOWN_STATE_UP) && (shutdown_state < HOSTSHUTDOWN_STATE_DOWN) )  
    {
        ioWrite(MCU_IO_MGR_SCK_LED,LOGIC_LEVEL_HIGH); // turn off the LED
        return;
    }
    
    unsigned long kRuntime = elapsed(&blink_started_at);
    
    // Remote will start with the lockout bit set so don't blink for that
    if (!(status_byt & ~(1<<HOST_LOCKOUT_STATUS) )) 
    {
        // blink half as fast when host is foreign
        if (host_is_foreign)
        {
            kRuntime = kRuntime >> 1;
        }
        
        if ( bootloader_started  && (kRuntime > BLINK_BOOTLD_DELAY) )
        {
            ioToggle(MCU_IO_MGR_SCK_LED);
            
            // next toggle 
            blink_started_at += BLINK_BOOTLD_DELAY; 
        }
        else if ( lockout_active  && (kRuntime > BLINK_LOCKOUT_DELAY) )
        {
            ioToggle(MCU_IO_MGR_SCK_LED);
            
            // next toggle 
            blink_started_at += BLINK_LOCKOUT_DELAY; 
        }
        else if ( host_active  && (kRuntime > BLINK_ACTIVE_DELAY) )
        {
            ioToggle(MCU_IO_MGR_SCK_LED);
            
            // next toggle 
            blink_started_at += BLINK_ACTIVE_DELAY; 
        }
        // else spin the loop
    }
    else
    {
        if ( (kRuntime > BLINK_STATUS_DELAY))
        {
            ioToggle(MCU_IO_MGR_SCK_LED);
            
            // next toggle 
            blink_started_at += BLINK_STATUS_DELAY; 
        }
    }
}

void check_Bootload_Time(void)
{
    if (bootloader_started)
    {
        unsigned long kRuntime = elapsed(&bootloader_started_at);
        
        if (!arduino_mode && (kRuntime > BOOTLOADER_ACTIVE))
        {
            connect_normal_mode();
            host_active =1;
            bootloader_started = 0;
        }
    }
}


// lockout needs to happoen for a long enough time to insure bootloading is finished,
void check_lockout(void)
{
    unsigned long kRuntime = elapsed(&lockout_started_at);
    
    if (!arduino_mode && ( lockout_active && (kRuntime > LOCKOUT_DELAY) ))
    {
        connect_normal_mode();

        host_active = 1;
        lockout_active =0;
    }
}
