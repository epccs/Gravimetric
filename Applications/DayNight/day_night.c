/*
day-night  is a library that is used to track the day and night for control functions. 
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
#include "../lib/timers_bsd.h"
#include "../lib/rpu_mgr.h"
#include "../lib/rpu_mgr_callback.h"
#include "../lib/adc_bsd.h"
#include "day_night.h"
#include "main.h"

volatile DAYNIGHT_STATE_t daynight_state; 
#define CHK_DAYNIGHT_DELAY 2000UL

static unsigned long daynight_serial_print_started_at;

// /0//day?
// report on daynight state machine, threshold and debounce settings, adc reading, and elapsed time since dayTmrStarted
void dnReport(unsigned long serial_print_delay_milsec)
{
    if ( (command_done == 10) )
    {
        daynight_serial_print_started_at = milliseconds();
        printf_P(PSTR("{\"state\":\"0x%X\","),daynight_state); // print a hex value
        command_done = 12;
        return;
    }
    else if ( (command_done == 12) ) 
    {
        int local_copy = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            local_copy = i2c_int_rwoff_access_cmd(DAYNIGHT_INT_CMD,DAYNIGHT_MORNING_THRESHOLD,0,&loop_state);
        }
        printf_P(PSTR("\"mor_threshold\":"));
        if (mgr_twiErrorCode)
        {
            printf_P(PSTR("\"err%d\","),mgr_twiErrorCode);
        }
        else
        {
            printf_P(PSTR("\"%u\","),local_copy);
        }
        command_done = 13;
        return;
    }
    else if ( (command_done == 13) ) 
    {
        int local_copy = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            local_copy = i2c_int_rwoff_access_cmd(DAYNIGHT_INT_CMD,DAYNIGHT_EVENING_THRESHOLD,0,&loop_state);
        }
        printf_P(PSTR("\"eve_threshold\":"));
        if (mgr_twiErrorCode)
        {
            printf_P(PSTR("\"err%d\","),mgr_twiErrorCode);
        }
        else
        {
            printf_P(PSTR("\"%u\","),local_copy);
        }
        command_done = 14;
        return;
    }
    else if ( (command_done == 14) ) 
    {
        int adc_reads = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            adc_reads = i2c_get_adc_from_manager(ADC_CH_MGR_ALT_V,&loop_state);
        }
        printf_P(PSTR("\"adc_alt_v\":\"%u\","),adc_reads);
        command_done = 15;
        return;
    }
    else if ( (command_done == 15) ) 
    {
        unsigned long local_copy = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            local_copy = i2c_ul_rwoff_access_cmd(DAYNIGHT_UL_CMD,DAYNIGHT_MORNING_DEBOUNCE,0,&loop_state);
        }
        printf_P(PSTR("\"mor_debounce\":"));
        if (mgr_twiErrorCode)
        {
            printf_P(PSTR("\"err%d\","),mgr_twiErrorCode);
        }
        else
        {
            printf_P(PSTR("\"%lu\","),local_copy);
        }
        command_done = 16;
        return;
    }
    else if ( (command_done == 16) ) 
    {
        unsigned long local_copy = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            local_copy = i2c_ul_rwoff_access_cmd(DAYNIGHT_UL_CMD,DAYNIGHT_EVENING_DEBOUNCE,0,&loop_state);
        }
        printf_P(PSTR("\"eve_debounce\":"));
        if (mgr_twiErrorCode)
        {
            printf_P(PSTR("\"err%d\","),mgr_twiErrorCode);
        }
        else
        {
            printf_P(PSTR("\"%lu\","),local_copy);
        }
        command_done = 17;
        return;
    }
    else if ( (command_done == 17) ) 
    {
        unsigned long local_copy = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            local_copy = i2c_ul_rwoff_access_cmd(DAYNIGHT_UL_CMD,DAYNIGHT_ELAPSED_TIMER,0,&loop_state);
        }
        printf_P(PSTR("\"dn_timer\":"));
        if (mgr_twiErrorCode)
        {
            printf_P(PSTR("\"err%d\","),mgr_twiErrorCode);
        }
        else
        {
            printf_P(PSTR("\"%lu\""),local_copy);
        }
        command_done = 24;
        return;
    }
    else if ( (command_done == 24) ) 
    {
        printf_P(PSTR("}\r\n"));
        command_done = 25;
        return;
    }
    else if ( (command_done == 25) ) 
    {
        unsigned long kRuntime= elapsed(&daynight_serial_print_started_at);
        if ((kRuntime) > (serial_print_delay_milsec))
        {
            command_done = 10; /* This keeps looping output forever (until a Rx char anyway) */
        }
    }
}

// /0/dnmthresh? [value]
// set daynight state machine daynight_morning_threshold
void dnMorningThreshold(void)
{
    if ( (command_done == 10) )
    {
        // if got argument[0] check its rage (it is used with 10 bit ADC)
        if (arg_count == 1)
        {
            if ( ( !( isdigit(arg[0][0]) ) ) || (atoi(arg[0]) < 0) || (atoi(arg[0]) >= (1<<10) ) )
            {
                printf_P(PSTR("{\"err\":\"DNMornThrshMax 1023\"}\r\n"));
                initCommandBuffer();
                return;
            }
            command_done = 11;
        }
        else
        {
            command_done = 12;
        }
        printf_P(PSTR("{\"mor_threshold\":"));
        return;
    }
    if ( (command_done == 11) ) 
    {
        int int_to_send = 0;
        if (arg_count == 1) int_to_send = atoi(arg[0]);
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            // update the manager, the old value is returned, but not needed
            i2c_int_rwoff_access_cmd(DAYNIGHT_INT_CMD,DAYNIGHT_MORNING_THRESHOLD+RW_WRITE_BIT,int_to_send,&loop_state);
        }
        command_done = 12;
        return;
    }
    if ( (command_done == 12) ) 
    {
        int int_to_get = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            int_to_get = i2c_int_rwoff_access_cmd(DAYNIGHT_INT_CMD,DAYNIGHT_MORNING_THRESHOLD,0,&loop_state);
        }
        printf_P(PSTR("\"%u\"}\r\n"),int_to_get);
        initCommandBuffer();
        return;
    }
}

// /0/dnethresh? [value]
// set daynight state machine daynight_evening_threshold
void dnEveningThreshold(void)
{
    if ( (command_done == 10) )
    {
        // if got argument[0] check its rage (it is used with 10 bit ADC)
        if (arg_count == 1)
        {
            if ( ( !( isdigit(arg[0][0]) ) ) || (atoi(arg[0]) < 0) || (atoi(arg[0]) >= (1<<10) ) )
            {
                printf_P(PSTR("{\"err\":\"DNEvenThrshMax 1023\"}\r\n"));
                initCommandBuffer();
                return;
            }
            command_done = 11;
        }
        else
        {
            command_done = 12;
        }
        printf_P(PSTR("{\"eve_threshold\":"));
        return;
    }
    if ( (command_done == 11) ) 
    {
        int int_to_send = 0;
        if (arg_count == 1) int_to_send = atoi(arg[0]);
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            // update the manager, the old value is returned, but not needed
            i2c_int_rwoff_access_cmd(DAYNIGHT_INT_CMD,DAYNIGHT_EVENING_THRESHOLD+RW_WRITE_BIT,int_to_send,&loop_state);
        }
        command_done = 12;
        return;
    }
    if ( (command_done == 12) ) 
    {
        int int_to_get = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            int_to_get = i2c_int_rwoff_access_cmd(DAYNIGHT_INT_CMD,DAYNIGHT_EVENING_THRESHOLD,0,&loop_state);
        }
        printf_P(PSTR("\"%u\"}\r\n"),int_to_get);
        initCommandBuffer();
        return;
    }
}

// /0/dnmdebounc? [value]
// set daynight state machine daynight_morning_debounce
void dnMorningDebounce(void)
{
    if ( (command_done == 10) )
    {
        // if got argument[0] check its rage (it is used with 10 bit ADC)
        if (arg_count == 1)
        {
            uint8_t arg0_outOfRange = 0; 
            if ( !isdigit(arg[0][0]) )
            {
                printf_P(PSTR("{\"err\":\"DNMorDebounc NaN\"}\r\n"));
                arg0_outOfRange = 1;
            }
            else
            {
                if (strtoul(arg[0], (char **)NULL, 10) < 8000UL)
                {
                    printf_P(PSTR("{\"err\":\"DNMorDebounc> 8000\"}\r\n"));
                    arg0_outOfRange = 1;
                }
                if (strtoul(arg[0], (char **)NULL, 10) > 3600000UL)
                {
                    printf_P(PSTR("{\"err\":\"DNMorDebounc< 3600000\"}\r\n"));
                    arg0_outOfRange = 1;
                }
            }
            if (arg0_outOfRange)
            {
                initCommandBuffer();
                return;
            }
            command_done = 11;
        }
        else
        {
            command_done = 12;
        }
        printf_P(PSTR("{\"mor_debounce\":"));
    }
    if ( (command_done == 11) ) 
    {
        unsigned long ul_to_send = 0;
        if (arg_count == 1) ul_to_send = strtoul(arg[0], (char **)NULL, 10);
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            // update the manager, the old value is returned, but not needed
            i2c_ul_rwoff_access_cmd(DAYNIGHT_UL_CMD,DAYNIGHT_MORNING_DEBOUNCE+RW_WRITE_BIT,ul_to_send,&loop_state);
        }
        command_done = 12;
    }
    if ( (command_done == 12) ) 
    {
        unsigned long ul_to_get = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            ul_to_get = i2c_ul_rwoff_access_cmd(DAYNIGHT_UL_CMD,DAYNIGHT_MORNING_DEBOUNCE,0,&loop_state);
        }
        printf_P(PSTR("\"%lu\""),ul_to_get);
        if(!ul_to_get)
        {
            printf_P(PSTR(",\"mgr_twierr\":\"%u\""),mgr_twiErrorCode);
        }
        printf_P(PSTR("}\r\n"));
        initCommandBuffer();
        return;
    }
}

// /0/dnedebounc? [value]
// set daynight state machine daynight_evening_debounce
void dnEveningDebounce(void)
{
    if ( (command_done == 10) )
    {
        // if got argument[0] check its rage (it is used with 10 bit ADC)
        if (arg_count == 1)
        {
            uint8_t arg0_outOfRange = 0; 
            if ( !isdigit(arg[0][0]) )
            {
                printf_P(PSTR("{\"err\":\"DNEveDebounc NaN\"}\r\n"));
                arg0_outOfRange = 1;
            }
            else
            {
                if (strtoul(arg[0], (char **)NULL, 10) < 8000UL)
                {
                    printf_P(PSTR("{\"err\":\"DNEveDebounc> 8000\"}\r\n"));
                    arg0_outOfRange = 1;
                }
                if (strtoul(arg[0], (char **)NULL, 10) > 3600000UL)
                {
                    printf_P(PSTR("{\"err\":\"DNEveDebounc< 3600000\"}\r\n"));
                    arg0_outOfRange = 1;
                }
            }
            if (arg0_outOfRange)
            {
                initCommandBuffer();
                return;
            }
            command_done = 11;
        }
        else
        {
            command_done = 12;
        }
        printf_P(PSTR("{\"eve_debounce\":"));
    }
    if ( (command_done == 11) ) 
    {
        unsigned long ul_to_send = 0;
        if (arg_count == 1) ul_to_send = strtoul(arg[0], (char **)NULL, 10);
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            // update the manager, the old value is returned, but not needed
            i2c_ul_rwoff_access_cmd(DAYNIGHT_UL_CMD,DAYNIGHT_EVENING_DEBOUNCE+RW_WRITE_BIT,ul_to_send,&loop_state);
        }
        command_done = 12;
    }
    if ( (command_done == 12) ) 
    {
        unsigned long ul_to_get = 0;
        TWI0_LOOP_STATE_t loop_state = TWI0_LOOP_STATE_INIT;
        while (loop_state != TWI0_LOOP_STATE_DONE)
        {
            ul_to_get = i2c_ul_rwoff_access_cmd(DAYNIGHT_UL_CMD,DAYNIGHT_EVENING_DEBOUNCE,0,&loop_state);
        }
        printf_P(PSTR("\"%lu\""),ul_to_get);
        if(!ul_to_get)
        {
            printf_P(PSTR(",\"mgr_twierr\":\"%u\""),mgr_twiErrorCode);
        }
        printf_P(PSTR("}\r\n"));
        initCommandBuffer();
        return;
    }
}

