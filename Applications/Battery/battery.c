/*
battery is a library that accesses the managers battery charging control.
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
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../lib/parse.h"
#include "../lib/rpu_mgr.h"
#include "../lib/timers_bsd.h"
#include "../Adc/references.h"
#include "../DayNight/day_night.h"
#include "main.h"
#include "battery.h"

static unsigned long battery_serial_print_started_at;
volatile BATTERYMGR_STATE_t batmgr_state;
uint8_t bm_callback_addr; 

// /0/bm
// enable the battery manager and the callback where it can tell me what it is doing.
void EnableBatMngCntl(void)
{
    if ( (command_done == 10) )
    {
        if (bm_callback_addr) 
        {
            bm_callback_addr = 0; // zero will disable battery manager
        }
        else
        {
            bm_callback_addr = I2C0_APP_ADDR;
        }
        
        i2c_battery_cmd(bm_callback_addr);
        printf_P(PSTR("{\"bat_en\":"));
        command_done = 11;
    }
    else if ( (command_done == 11) )
    {
        if (bm_callback_addr)
        {
            printf_P(PSTR("\"ON\""));
        }
        else
        {
            printf_P(PSTR("\"OFF\""));
        }
        command_done = 12;
    }
    else if ( (command_done == 12) )
    {
        printf_P(PSTR("}\r\n"));
        initCommandBuffer();
    }
    else
    {
        initCommandBuffer();
    }
}

// /0/bmcntl?
// report on battery manager, battery_low_limit, battery_high_limit
void ReportBatMngCntl(unsigned long serial_print_delay_milsec)
{
    if ( (command_done == 10) )
    {
        battery_serial_print_started_at = milliseconds();
        command_done = 11;
    }
    if ( (command_done == 11) )
    {
        printf_P(PSTR("{\"bm_state\":\"0x%X\","),batmgr_state); // print a hex value
        command_done = 12;
    }
    else if ( (command_done == 12) ) 
    {
        int local_copy = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            local_copy = i2c_int_access_cmd(CHARGE_BATTERY_LOW,0,&loop_state);
        }
        printf_P(PSTR("\"bat_low_lim\":\"%u\","),local_copy);
        command_done = 13;
    }
    else if ( (command_done == 13) ) 
    {
        int local_copy = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            local_copy = i2c_int_access_cmd(CHARGE_BATTERY_HIGH,0,&loop_state);
        }
        printf_P(PSTR("\"bat_high_lim\":\"%u\","),local_copy);
        command_done = 14;
    }
    else if ( (command_done == 14) ) 
    {
        int adc_reads = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            adc_reads = i2c_get_adc_from_manager(ADC_CH_MGR_PWR_V,&loop_state);
        }
        printf_P(PSTR("\"adc_pwr_v\":\"%u\","),adc_reads);
        command_done = 15;
    }
    else if ( (command_done == 15) ) 
    {
        int adc_reads = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            adc_reads = i2c_get_adc_from_manager(ADC_CH_MGR_ALT_V,&loop_state);
        }
        printf_P(PSTR("\"adc_alt_v\":\"%u\","),adc_reads);
        command_done = 16;
    }
    else if ( (command_done == 16) ) 
    {
        unsigned long local_copy = i2c_ul_access_cmd(CHARGE_BATTERY_PWM,0);
        printf_P(PSTR("\"pwm_timer\":\"%lu\","),local_copy); // alt_pwm_accum_charge_time
        command_done = 17;
    }
    else if ( (command_done == 17) ) 
    {
        unsigned long local_copy = i2c_ul_access_cmd(DAYNIGHT_TIMER,0);
        printf_P(PSTR("\"dn_timer\":\"%lu\""),local_copy); // elapsed_time_since_dayTmrStarted
        command_done = 24;
    }
    else if ( (command_done == 24) ) 
    {
        printf_P(PSTR("}\r\n"));
        command_done = 25;
    }
    else if ( (command_done == 25) ) 
    {
        unsigned long kRuntime= elapsed(&battery_serial_print_started_at);
        if ((kRuntime) > (serial_print_delay_milsec))
        {
            battery_serial_print_started_at += serial_print_delay_milsec;
            command_done = 11; /* This keeps looping output forever (until a Rx char anyway) */
        }
    }
} 

// /0/bmlow? [value]
// read [set] battery manager battery_low_limit
void BatMngLowLimit(void)
{
    if ( (command_done == 10) )
    {
        // if got argument[0] check its rage (it is used with 10 bit ADC)
        if (arg_count == 1)
        {
            if ( ( !( isdigit(arg[0][0]) ) ) || (atoi(arg[0]) < 0) || (atoi(arg[0]) >= (1<<10) ) )
            {
                printf_P(PSTR("{\"err\":\"BMLowLimMax 1023\"}\r\n"));
                initCommandBuffer();
                return;
            }
            command_done = 11;
        }
        else
        {
            command_done = 12;
        }
        printf_P(PSTR("{\"bat_low_lim\":"));
    }
    if ( (command_done == 11) ) 
    {
        int int_to_send = 0;
        if (arg_count == 1) int_to_send = atoi(arg[0]);
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            // update the manager, the old value is returned, but not needed
            i2c_int_access_cmd(CHARGE_BATTERY_LOW,int_to_send,&loop_state);
        }
        command_done = 12;
    }
    if ( (command_done == 12) ) 
    {
        int int_to_send = 0;
        int int_to_get = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            int_to_get = i2c_int_access_cmd(CHARGE_BATTERY_LOW,int_to_send,&loop_state);
        }
        printf_P(PSTR("\"%u\"}\r\n"),int_to_get);
        initCommandBuffer();
        return;
    }
}

// /0/bmhigh? [value]
// set battery manager battery_high_limit
void BatMngHighLimit(void)
{
    if ( (command_done == 10) )
    {
        // if got argument[0] check its rage (it is used with 10 bit ADC)
        if (arg_count == 1)
        {
            if ( ( !( isdigit(arg[0][0]) ) ) || (atoi(arg[0]) < 0) || (atoi(arg[0]) >= (1<<10) ) )
            {
                printf_P(PSTR("{\"err\":\"BMHighLimMax 1023\"}\r\n"));
                initCommandBuffer();
                return;
            }
            command_done = 11;
        }
        else
        {
            command_done = 12;
        }
        printf_P(PSTR("{\"bat_high_lim\":"));
    }
    if ( (command_done == 11) ) 
    {
        int int_to_send = 0;
        if (arg_count == 1) int_to_send = atoi(arg[0]);
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            // update the manager, the old value is returned, but not needed
            i2c_int_access_cmd(CHARGE_BATTERY_HIGH,int_to_send,&loop_state);
        }
        command_done = 12;
    }
    if ( (command_done == 12) ) 
    {
        int int_to_send = 0;
        int int_to_get = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            int_to_get = i2c_int_access_cmd(CHARGE_BATTERY_HIGH,int_to_send,&loop_state);
        }
        printf_P(PSTR("\"%u\"}\r\n"),int_to_get);
        initCommandBuffer();
        return;
    }
}
