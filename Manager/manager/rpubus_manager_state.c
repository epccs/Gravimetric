/*
state for RPUBUS manager
Copyright (C) 2019 Ronald Sutherland

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <util/delay.h>
#include <avr/io.h>
#include "../lib/timers.h"
#include "../lib/uart.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"
#include "rpubus_manager_state.h"

unsigned long blink_started_at;
unsigned long lockout_started_at;
unsigned long bootloader_started_at;
unsigned long shutdown_started_at;

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
        digitalWrite(RX_DE, LOW); // disallow RX pair driver to enable if FTDI_TX is low
        digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
        if(local_mcu_is_rpu_aware)
        {
            digitalWrite(TX_DE, HIGH); // allow TX pair driver to enable if TX (from MCU) is low
        }
        else
        {
            digitalWrite(TX_DE, LOW); // disallow TX pair driver to enable if TX (from MCU) is low
        }
        digitalWrite(TX_nRE, HIGH);  // disable TX pair recevior to output to FTDI_RX input
    }

     // connect both the local mcu and host/ftdi uart if mcu is rpu aware, otherwise block MCU from using the TX pair
    else
    {
        digitalWrite(RX_DE, HIGH); // allow RX pair driver to enable if FTDI_TX is low
        digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
        if(local_mcu_is_rpu_aware)
        {
            digitalWrite(TX_DE, HIGH); // allow TX pair driver to enable if TX (from MCU) is low
        }
        else
        {
            digitalWrite(TX_DE, LOW); // disallow TX pair driver to enable if TX (from MCU) is low
        }
        digitalWrite(TX_nRE, LOW);  // enable TX pair recevior to output to FTDI_RX input
    }
}

void connect_bootload_mode(void)
{
    // connect the remote host and local mcu
    if (host_is_foreign)
    {
        digitalWrite(RX_DE, LOW); // disallow RX pair driver to enable if FTDI_TX is low
        digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
        digitalWrite(TX_DE, HIGH); // allow TX pair driver to enable if TX (from MCU) is low
        digitalWrite(TX_nRE, HIGH);  // disable TX pair recevior to output to FTDI_RX input
    }
    
    // connect the local host and local mcu
    else
    {
        digitalWrite(RX_DE, HIGH); // allow RX pair driver to enable if FTDI_TX is low
        digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
        digitalWrite(TX_DE, HIGH); // allow TX pair driver to enable if TX (from MCU) is low
        digitalWrite(TX_nRE, LOW);  // enable TX pair recevior to output to FTDI_RX input
    }
}

void connect_lockout_mode(void)
{
    // lockout everything
    if (host_is_foreign)
    {
        digitalWrite(RX_DE, LOW); // disallow RX pair driver to enable if FTDI_TX is low
        digitalWrite(RX_nRE, HIGH);  // disable RX pair recevior to output to local MCU's RX input
        digitalWrite(TX_DE, LOW); // disallow TX pair driver to enable if TX (from MCU) is low
        digitalWrite(TX_nRE, HIGH);  // disable TX pair recevior to output to FTDI_RX input
    }
    
    // lockout MCU, but not host
    else
    {
        digitalWrite(RX_DE, HIGH); // allow RX pair driver to enable if FTDI_TX is low
        digitalWrite(RX_nRE, HIGH);  // disable RX pair recevior to output to local MCU's RX input
        digitalWrite(TX_DE, LOW); // disallow TX pair driver to enable if TX (from MCU) is low
        digitalWrite(TX_nRE, LOW);  // enable TX pair recevior to output to FTDI_RX input
    }
}

// blink if the host is active, fast blink if status_byt, slow blink in lockout
void blink_on_activate(void)
{
    if (shutdown_detected) // do not blink,  power usage needs to be very stable to tell if the host has haulted. 
    {
        return;
    }
    
    unsigned long kRuntime = millis() - blink_started_at;
    
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
            digitalToggle(LED_BUILTIN);
            
            // next toggle 
            blink_started_at += BLINK_BOOTLD_DELAY; 
        }
        else if ( lockout_active  && (kRuntime > BLINK_LOCKOUT_DELAY) )
        {
            digitalToggle(LED_BUILTIN);
            
            // next toggle 
            blink_started_at += BLINK_LOCKOUT_DELAY; 
        }
        else if ( host_active  && (kRuntime > BLINK_ACTIVE_DELAY) )
        {
            digitalToggle(LED_BUILTIN);
            
            // next toggle 
            blink_started_at += BLINK_ACTIVE_DELAY; 
        }
        // else spin the loop
    }
    else
    {
        if ( (kRuntime > BLINK_STATUS_DELAY))
        {
            digitalToggle(LED_BUILTIN);
            
            // next toggle 
            blink_started_at += BLINK_STATUS_DELAY; 
        }
    }
}

void check_Bootload_Time(void)
{
    if (bootloader_started)
    {
        unsigned long kRuntime = millis() - bootloader_started_at;
        
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
    unsigned long kRuntime = millis() - lockout_started_at;
    
    if (!arduino_mode && ( lockout_active && (kRuntime > LOCKOUT_DELAY) ))
    {
        connect_normal_mode();

        host_active = 1;
        lockout_active =0;
    }
}


void check_shutdown(void)
{
    if (shutdown_started)
    {
        unsigned long kRuntime = millis() - shutdown_started_at;
        
        if ( kRuntime > SHUTDOWN_TIME)
        {
            pinMode(SHUTDOWN, INPUT);
            digitalWrite(SHUTDOWN, HIGH); // trun on a weak pullup 
            shutdown_started = 0; // set with I2C command 5
            shutdown_detected = 1; // clear when reading with I2C command 4
        }
    }
    else
        if (!shutdown_detected) 
        { 
            // I2C cmd set shutdown_started =1 and set shutdown_detected = 0
            // but if it is a manual event it can have a debounce time
            if( !digitalRead(SHUTDOWN) ) 
            {
                pinMode(SHUTDOWN, OUTPUT);
                digitalWrite(SHUTDOWN, LOW);
                pinMode(LED_BUILTIN, OUTPUT);
                digitalWrite(LED_BUILTIN, HIGH);
                shutdown_detected = 0; // set after SHUTDOWN_TIME timer runs
                shutdown_started = 1; // it is cleared after SHUTDOWN_TIME timer runs
                shutdown_started_at = millis();
            }
        }
}
