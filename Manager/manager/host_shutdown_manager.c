/*
Host shutdown manager
Copyright (C) 2020 Ronald Sutherland

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
#include "../lib/adc_bsd.h"
#include "../lib/twi0_bsd.h"
#include "../lib/io_enum_bsd.h"
#include "i2c_callback.h"
#include "daynight_state.h"
#include "battery_manager.h"
#include "host_shutdown_limits.h"
#include "host_shutdown_manager.h"

HOSTSHUTDOWN_STATE_t shutdown_state;

unsigned long shutdown_kRuntime;
unsigned long shutdown_started_at; // time when BCM6 was pulled low
unsigned long shutdown_halt_chk_at; // time when current on PWR_I got bellow the expected level
unsigned long shutdown_wearleveling_done_at; // time when current on PWR_I got stable

uint8_t shutdown_callback_address;
uint8_t shutdown_state_callback_cmd;
TWI0_LOOP_STATE_t loop_state;

uint8_t fail_wip;
uint8_t resume_bm_enable;
int last_pwr_i;

// if the manager SHUTDOWN pin PBO is pulled low then the host should halt.
// the manager can turn off power after verifying the host is down 
// shutdown_callback_address must be set for application to get events
void check_if_host_should_be_on(void)
{
    // TWI waiting to finish (ignore changes while TWI is waiting)
    if (i2c_callback_waiting(&loop_state)) return; 
    
    // if somthing else is using twi then my loop_state will allow getting to this, but I want to skip the state machine
    if (i2c_is_in_use) return;

    // if host_shutdown limits are changing skip this state machine
    if (shutdown_limit_loaded > HOSTSHUTDOWN_LIM_DEFAULT) return;

    int pwr_i = adcAtomic(ADC_CH_PWR_I);
    unsigned long kRuntime = elapsed(&shutdown_kRuntime);
    switch (shutdown_state)
    {
    case HOSTSHUTDOWN_STATE_UP: // If manager is held in reset the host will power up. This default state is a running host. 
        if (!ioRead(MCU_IO_SHUTDOWN)) // If the manual button is pushed for two seconds pull this pin low.
        {
            if (kRuntime > 2000UL) 
            {
                ioDir(MCU_IO_SHUTDOWN, DIRECTION_OUTPUT);
                ioWrite(MCU_IO_SHUTDOWN, LOGIC_LEVEL_LOW);
                shutdown_kRuntime = milliseconds();
                shutdown_state = HOSTSHUTDOWN_STATE_HALT;
                if (shutdown_callback_address && shutdown_state_callback_cmd)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(shutdown_callback_address, shutdown_state_callback_cmd, shutdown_state, &loop_state); // update application
                }
            }
        }
        else
        {
            shutdown_kRuntime = milliseconds(); // if manual switch is open reset the timer
        }
        break;


    case HOSTSHUTDOWN_STATE_SW_HALT: // this is set by an I2C or UART command it is not be reported by callback
        ioDir(MCU_IO_SHUTDOWN, DIRECTION_OUTPUT);
        ioWrite(MCU_IO_SHUTDOWN, LOGIC_LEVEL_LOW);
        shutdown_kRuntime = milliseconds();
        shutdown_state = HOSTSHUTDOWN_STATE_HALT;
        if (shutdown_callback_address && shutdown_state_callback_cmd)
        {
            if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
            i2c_callback(shutdown_callback_address, shutdown_state_callback_cmd, shutdown_state, &loop_state); // update application
        }
        break;

    case HOSTSHUTDOWN_STATE_HALT: // a manual or software halt is starting
        if (kRuntime > 200UL) // hold the shutdown pin low for 200mSec
        {
            resume_bm_enable = enable_bm_callback_address;
            enable_bm_callback_address = 0;
            shutdown_started_at = shutdown_kRuntime;
            shutdown_kRuntime = milliseconds(); // start time to live timer
            shutdown_state = HOSTSHUTDOWN_STATE_CURR_CHK;
            if (shutdown_callback_address && shutdown_state_callback_cmd)
            {
                if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                i2c_callback(shutdown_callback_address, shutdown_state_callback_cmd, shutdown_state, &loop_state); // update application
            }
        }
        break;

    case HOSTSHUTDOWN_STATE_CURR_CHK: // the battery manager was disable, now wait for CURR on PWR_I
        if (kRuntime > shutdown_ttl_limit) // time out
        {
            shutdown_state = HOSTSHUTDOWN_STATE_HALTTIMEOUT_RESET_APP;
            if (shutdown_callback_address && shutdown_state_callback_cmd)
            {
                if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                i2c_callback(shutdown_callback_address, shutdown_state_callback_cmd, shutdown_state, &loop_state); // update application
            }
        }
        if (pwr_i < shutdown_halt_curr_limit)
        {
            shutdown_halt_chk_at = milliseconds(); // time when current on PWR_I got bellow the expected level
            shutdown_state = HOSTSHUTDOWN_STATE_AT_HALT_CURR;
            if (shutdown_callback_address && shutdown_state_callback_cmd)
            {
                if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                i2c_callback(shutdown_callback_address, shutdown_state_callback_cmd, shutdown_state, &loop_state); // update application
            }
        }
        break;

    case HOSTSHUTDOWN_STATE_HALTTIMEOUT_RESET_APP: // If the hault current is not seen after a timeout then what?
        // options: HOSTSHUTDOWN_STATE_FAIL 
        //          hold the appliction in reset and try again?
        //          HOSTSHUTDOWN_STATE_AT_HALT_CURR will power off anyway.
        shutdown_state = HOSTSHUTDOWN_STATE_FAIL;
        if (shutdown_callback_address && shutdown_state_callback_cmd)
        {
            if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
            i2c_callback(shutdown_callback_address, shutdown_state_callback_cmd, shutdown_state, &loop_state); // update application
        }
        break;

    case HOSTSHUTDOWN_STATE_AT_HALT_CURR: // PWR_I is bellow the expected level, it is differnt for each model of R-Pi
        shutdown_kRuntime = milliseconds(); // start timer for delay after we are at halt current
        shutdown_state = HOSTSHUTDOWN_STATE_DELAY;
        if (shutdown_callback_address && shutdown_state_callback_cmd)
        {
            if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
            i2c_callback(shutdown_callback_address, shutdown_state_callback_cmd, shutdown_state, &loop_state); // update application
        }
        break;

    case HOSTSHUTDOWN_STATE_DELAY: // this state is a delay in miliseconds
        if (kRuntime > shutdown_delay_limit) // delay
        {
            shutdown_state = HOSTSHUTDOWN_STATE_WEARLEVELING;
            if (shutdown_callback_address && shutdown_state_callback_cmd)
            {
                if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                i2c_callback(shutdown_callback_address, shutdown_state_callback_cmd, shutdown_state, &loop_state); // update application
            }
        }
        break;

    case HOSTSHUTDOWN_STATE_WEARLEVELING: // pwr_i has to be stable for a time
        if ( (pwr_i >= (last_pwr_i - 1)) && (pwr_i <= (last_pwr_i + 1)) )
        {
            if (kRuntime > shutdown_wearleveling_limit) 
            {
                shutdown_state = HOSTSHUTDOWN_STATE_DOWN;
                ioWrite(MCU_IO_PIPWR_EN, LOGIC_LEVEL_LOW); // power down the SBC
                ioDir(MCU_IO_PIPWR_EN, DIRECTION_OUTPUT);
                ioDir(MCU_IO_SHUTDOWN, DIRECTION_INPUT);
                ioWrite(MCU_IO_SHUTDOWN, LOGIC_LEVEL_HIGH); // enable pull up on old AVR Mega parts
                shutdown_wearleveling_done_at = milliseconds();
                enable_bm_callback_address = resume_bm_enable; // restore battery manager enable
                if (shutdown_callback_address && shutdown_state_callback_cmd)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(shutdown_callback_address, shutdown_state_callback_cmd, shutdown_state, &loop_state); // update application
                }
            }
        }
        else
        {
            last_pwr_i = pwr_i;
            shutdown_kRuntime = milliseconds(); // restart timer
        }
        break;

    case HOSTSHUTDOWN_STATE_DOWN: // SBC is powered down
        if (!ioRead(MCU_IO_SHUTDOWN)) // If the manual button is pushed for two seconds then get ready to restart the SBC.
        {
            if (kRuntime > 2000UL) 
            {
                shutdown_state = HOSTSHUTDOWN_STATE_RESTART;
                if (shutdown_callback_address && shutdown_state_callback_cmd)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(shutdown_callback_address, shutdown_state_callback_cmd, shutdown_state, &loop_state); // update application
                }
            }
        }
        else
        {
            shutdown_kRuntime = milliseconds();
        }
        break;

    case HOSTSHUTDOWN_STATE_RESTART: // restart when the shutdown switch is clear
        if (ioRead(MCU_IO_SHUTDOWN)) // If the manual button is open for two seconds power the SBC.
        {
            if (kRuntime > 2000UL) 
            {
                shutdown_state = HOSTSHUTDOWN_STATE_RESTART_DLY;
                ioDir(MCU_IO_PIPWR_EN, DIRECTION_INPUT);
                ioWrite(MCU_IO_PIPWR_EN, LOGIC_LEVEL_HIGH); // power up the SBC
                ioDir(MCU_IO_SHUTDOWN, DIRECTION_OUTPUT);
                ioWrite(MCU_IO_SHUTDOWN, LOGIC_LEVEL_HIGH); // lockout manual shutdown switch
                if (shutdown_callback_address && shutdown_state_callback_cmd)
                {
                    if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                    i2c_callback(shutdown_callback_address, shutdown_state_callback_cmd, shutdown_state, &loop_state); // update application
                }
            }
        }
        else
        {
            shutdown_kRuntime = milliseconds(); // if manual switch is pushed reset the timer
        }
        break;

    case HOSTSHUTDOWN_STATE_RESTART_DLY: // delay after restart so the host is UP
        if (kRuntime > 60000UL) 
        {
            shutdown_state = HOSTSHUTDOWN_STATE_UP;
            ioDir(MCU_IO_SHUTDOWN, DIRECTION_INPUT);
            ioWrite(MCU_IO_SHUTDOWN, LOGIC_LEVEL_HIGH); // enable manual shutdown switch (with weak pull up)
            if (shutdown_callback_address && shutdown_state_callback_cmd)
            {
                if (loop_state == TWI0_LOOP_STATE_RAW) loop_state = TWI0_LOOP_STATE_INIT;
                i2c_callback(shutdown_callback_address, shutdown_state_callback_cmd, shutdown_state, &loop_state); // update application
            }
        }
        break;

    case HOSTSHUTDOWN_STATE_FAIL: // somthing went wrong
        // the managers PIPWR_EN pin will be pulled low, this may damage the SD card.
        ioWrite(MCU_IO_PIPWR_EN, LOGIC_LEVEL_LOW); // power down the SBC
        ioDir(MCU_IO_PIPWR_EN, DIRECTION_OUTPUT);
        ioDir(MCU_IO_SHUTDOWN, DIRECTION_OUTPUT);
        ioWrite(MCU_IO_SHUTDOWN, LOGIC_LEVEL_LOW);
        break;

    default:
        break;
    }
}