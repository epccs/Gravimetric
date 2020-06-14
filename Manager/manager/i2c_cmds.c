/*
i2c_cmds is a i2c manager slave commands library in the form of a function pointer array  
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
#include <string.h>
#include <avr/io.h>
#include "../lib/timers_bsd.h"
#include "../lib/twi0_bsd.h"
#include "../lib/uart0_bsd.h"
#include "../lib/adc_bsd.h"
#include "../lib/io_enum_bsd.h"
#include "main.h"
#include "rpubus_manager_state.h"
#include "dtr_transmition.h"
#include "i2c_cmds.h"
#include "adc_burst.h"
#include "references.h"
#include "daynight_limits.h"
#include "daynight_state.h"
#include "battery_manager.h"
#include "battery_limits.h"
#include "host_shutdown_manager.h"
#include "host_shutdown_limits.h"
#include "calibration_limits.h"

uint8_t i2c0Buffer[I2C_BUFFER_LENGTH];
uint8_t i2c0BufferLength = 0;

// called when I2C data is received. 
void receive_i2c_event(uint8_t* inBytes, uint8_t numBytes) 
{
    // table of pointers to functions that are selected by the i2c cmmand byte
    static void (*pf[GROUP][MGR_CMDS])(uint8_t*) = 
    {
        {fnMgrAddr, fnStatus, fnBootldAddr, fnArduinMode, fnHostShutdwnMgr, fnHostShutdwnCurrLim, fnHostShutdwnULAccess, fnNull},
        {fnBatteryMgr, fnNull, fnBatChrgLowLim, fnBatChrgHighLim, fnRdBatChrgTime, fnMorningThreshold, fnEveningThreshold, fnDayNightState},
        {fnAnalogRead, fnCalibrationRead, fnNull, fnNull, fnRdTimedAccum, fnNull, fnReferance, fnNull},
        {fnStartTestMode, fnEndTestMode, fnRdXcvrCntlInTestMode, fnWtXcvrCntlInTestMode, fnMorningDebounce, fnEveningDebounce, fnDayNightTimer, fnNull}
    };

    // i2c will echo's back what was sent (plus modifications) with transmit event
    uint8_t i;
    for(i = 0; i < numBytes; ++i)
    {
        i2c0Buffer[i] = inBytes[i];    
    }
    if(i < I2C_BUFFER_LENGTH) i2c0Buffer[i+1] = 0; // room for null
    i2c0BufferLength = numBytes;

    // my i2c commands size themselfs with data, so at least two bytes (e.g., cmd + one_data_byte)
    if(i2c0BufferLength <= 1) 
    {
        i2c0Buffer[0] = 0xFF; // error code for small size.
        return; // not valid, do nothing just echo.
    }

    // mask the group bits (4..7) so they are alone then roll those bits to the left so they can be used as an index.
    uint8_t group;
    group = (i2c0Buffer[0] & 0xF0) >> 4;
     if(group >= GROUP) 
     {
        i2c0Buffer[0] = 0xFE; // error code for bad group.
        return; 
     }

    // mask the command bits (0..3) so they can be used as an index.
    uint8_t command;
    command = i2c0Buffer[0] & 0x0F;
    if(command >= MGR_CMDS) 
    {
        i2c0Buffer[0] = 0xFD; // error code for bad command.
        return; // not valid, do nothing but echo error code.
    }

    /* Call the command function and return */
    (* pf[group][command])(i2c0Buffer);
    return;	
}

void transmit_i2c_event(void) 
{
    // respond with an echo of the last message sent
    uint8_t return_code = twi0_fillSlaveTxBuffer(i2c0Buffer, i2c0BufferLength);
    if (return_code != 0)
        status_byt &= (1<<DTR_I2C_TRANSMIT_FAIL);
}

/********* MULTI-POINT MODE ***********
  *    each manager knows its address.
  *    each of the multiple controlers has a manager.
  *    the manager connects the controler to the bus when the address is read.
  *    each manager has a bootload address.
  *    the manager broadcast the bootload address when the host serial is active (e.g., nRTS) 
  *    all managers lockout serial except the address to bootload and the host */

// I2C command to access manager address and set RPU_NORMAL_MODE
// if given a valid address (ASCII 48..122) it will save that rather than setting normal mode.
void fnMgrAddr(uint8_t* i2cBuffer)
{
    uint8_t tmp_addr = i2cBuffer[1];
    i2cBuffer[1] = rpu_address; // ASCII values in range 0x30..0x7A. e.g.,'1' is 0x31
    if ( (tmp_addr>='0') && (tmp_addr<='z') ) 
    {
        rpu_address = tmp_addr;
        write_rpu_address_to_eeprom = 1;
        return;
    }
    local_mcu_is_rpu_aware =1; 
    // end the local mcu lockout. 
    if (localhost_active) 
    {
        // If the local host is active then broadcast on DTR pair
        uart_started_at = milliseconds();
        uart_output = RPU_NORMAL_MODE;
        printf("%c%c", uart_output, ( (~uart_output & 0x0A) << 4 | (~uart_output & 0x50) >> 4 ) ); 
        uart_has_TTL = 1; // causes host_is_foreign to be false
    }
    else 
        if (bootloader_started)
        {
            // If the bootloader_started has not timed out yet broadcast on DTR pair
            uart_started_at = milliseconds();
            uart_output = RPU_NORMAL_MODE;
            printf("%c%c", uart_output, ( (~uart_output & 0x0A) << 4 | (~uart_output & 0x50) >> 4 ) ); 
            uart_has_TTL = 0; // causes host_is_foreign to be true, so local DTR/RTS is not accepted
        } 
        else
        {
            lockout_started_at = milliseconds() - LOCKOUT_DELAY;
            bootloader_started_at = milliseconds() - BOOTLOADER_ACTIVE;
        }
        
}

//I2C command to access manager address (used with SMBus in place of above)
void fnMgrAddrQuietly(uint8_t* i2cBuffer)
{
    uint8_t tmp_addr = i2cBuffer[1];
    i2cBuffer[1] = rpu_address; // ASCII values in range 0x30..0x7A. e.g.,'1' is 0x31
    if ( (tmp_addr>='0') && (tmp_addr<='z') ) 
    {
        rpu_address = tmp_addr;
        write_rpu_address_to_eeprom = 1;
        return;
    }
}

// I2C command to access manager STATUS
void fnStatus(uint8_t* i2cBuffer)
{
    uint8_t tmp_status = i2cBuffer[1];
    i2cBuffer[1] = status_byt & 0x0F; // bits 0..3

    // if update bit 7 is set then change the status bits and related things
    if (tmp_status & 0x80)
    {
        status_byt = i2cBuffer[1] & 0x0F; // set bits 0..3
    }
}

// I2C command to access bootload address sent when HOST_nRTS toggles
void fnBootldAddr(uint8_t* i2cBuffer)
{
    uint8_t tmp_addr = i2cBuffer[1];

    // ASCII values in range 0x30..0x7A. e.g.,'1' is 0x31
    if ( (tmp_addr>='0') && (tmp_addr<='z') ) 
    {
        bootloader_address = tmp_addr;
    }

    i2cBuffer[1] = bootloader_address;
}

// I2C command to access arduino_mode
// read the local address to send a byte on DTR for RPU_NORMAL_MODE
// in arduino_mode LOCKOUT_DELAY and BOOTLOADER_ACTIVE will last forever after the HOST_nRTS toggles
void fnArduinMode(uint8_t* i2cBuffer)
{
    if (i2cBuffer[1] == 1)
    {
        if (!arduino_mode_started)
        {
            uart_started_at = milliseconds();
            uart_output = RPU_ARDUINO_MODE;
            printf("%c%c", uart_output, ( (~uart_output & 0x0A) << 4 | (~uart_output & 0x50) >> 4 ) ); 
            uart_has_TTL = 1; // causes host_is_foreign to be false
            arduino_mode_started = 1; // it is cleared by check_uart where arduino_mode is set
            arduino_mode = 0; // system wide state is set by check_uart when RPU_ARDUINO_MODE seen
        } 
    } // ignore everything but the command

    i2cBuffer[1] = arduino_mode; // ignore everything but the command
}

// I2C command to enable host shutdown manager and set a i2c callback address for batmgr_state when command command byte is > zero.
// The manager operates as an i2c master and addresses the application MCU as a slave to update when events occur.
// I2C: byte[0] = 4, 
//      byte[1] = shutdown_callback_address, 
//      byte[2] = shutdown_state_callback_cmd,
//      byte[3] = bring host UP[1..255] / take host DOWN[0].
void fnHostShutdwnMgr(uint8_t* i2cBuffer)
{
    shutdown_callback_address = i2cBuffer[1]; // non-zero is the i2c slave address used for callback
    shutdown_state_callback_cmd = i2cBuffer[2]; // however the callback will only happen if this value is > zero
    if (i2cBuffer[3]) // bring host UP
    {
        if (shutdown_state == HOSTSHUTDOWN_STATE_DOWN)  // host must be down to bring up
        {
            shutdown_state = HOSTSHUTDOWN_STATE_RESTART;
            ioDir(MCU_IO_SHUTDOWN, DIRECTION_INPUT);
            ioWrite(MCU_IO_SHUTDOWN, LOGIC_LEVEL_HIGH); // enable pull up
        }
    }
    else // take host DOWN
    {
        if (shutdown_state == HOSTSHUTDOWN_STATE_UP) // host must be up to take down
        {
            shutdown_state = HOSTSHUTDOWN_STATE_SW_HALT;
        }
    }
}

// I2C command to access shutdown_halt_curr_limit
// befor host shutdown is done PWR_I current must be bellow this limit.
// I2C: byte[0] = 5, 
//      byte[1] = bit 7 clear is read/bit 7 set is write, 
//      byte[2] = shutdown_halt_curr_limit:high_byte, 
//      byte[3] = shutdown_halt_curr_limit:low_byte.
void fnHostShutdwnCurrLim(uint8_t* i2cBuffer)
{
    uint8_t write = i2cBuffer[1] & 0x80; // read if bit 7 is clear, write if bit 7 is set
    uint8_t offset = i2cBuffer[1] & 0x7F; // 0 is shutdown_halt_curr_limit
    // shutdown_halt_curr_limit is an int e.g., two bytes
    // save the new_value
    int new_value = 0;
    new_value += ((int)i2cBuffer[2])<<8; // high_byte
    new_value += ((int)i2cBuffer[3]); // low_byte

    int old_value = 0;
    switch (offset)
    {
    case 0:
        old_value = shutdown_halt_curr_limit;
        break;
    case 1:
        old_value = 1023; // a test value to return
        break;

    default:
        break;
    }


    // swap the return value with shutdown_halt_curr_limit that is in use
    i2cBuffer[2] =  ( (0xFF00 & old_value) >>8 ); 
    i2cBuffer[3] =  ( (0x00FF & old_value) ); 

    // keep the new_value and mark shutdown_limit_loaded to save in EEPROM
    if (write)
    {
        switch (offset)
        {
        case 0:
            if (IsValidShtDwnHaltCurr(&new_value))
            {
                shutdown_halt_curr_limit = new_value;
                shutdown_limit_loaded = HOSTSHUTDOWN_LIM_HALT_CURR_TOSAVE; // main loop will save to eeprom
            } 
            break;

        default:
            break;
        }
    }
}

// I2C command to access shutdown_[halt_ttl_limit|delay_limit|wearleveling_limit|kRuntime|started_at|halt_chk_at|wearleveling_done_at]
// shutdown_ttl_limit
// befor host shutdown is done PWR_I current must be bellow this limit.
// I2C: byte[0] = 6, 
//      byte[1] = bit 7 is read/write 
//                bits 6..0 is offset to shutdown_[halt_ttl_limit|delay_limit|wearleveling_limit|kRuntime|started_at|halt_chk_at|wearleveling_done_at],
//      byte[2] = bits 32..24 of shutdown_[halt_ttl_limit|delay_limit|wearleveling_limit...],
//      byte[3] = bits 23..16,
//      byte[4] = bits 15..8,
//      byte[5] = bits 7..0,
void fnHostShutdwnULAccess(uint8_t* i2cBuffer)
{
    uint8_t write = i2cBuffer[1] & 0x80; // read if bit 7 is clear, write if bit 7 is set
    uint8_t offset = i2cBuffer[1] & 0x7F; // 0 is halt_ttl_limit, 1 is delay_limit...

    // save the new_value
    uint32_t new_value = 0;
    new_value += ((uint32_t)i2cBuffer[2])<<24; // high_byte
    new_value += ((uint32_t)i2cBuffer[3])<<16; // bits 23..16
    new_value += ((uint32_t)i2cBuffer[4])<<8; // bits 15..8
    new_value += ((uint32_t)i2cBuffer[5]); // low_byte

    uint32_t old_value = 0;
    switch (offset)
    {
    case 0:
        old_value = shutdown_ttl_limit;
        break;
    case 1:
        old_value = shutdown_delay_limit;
        break;
    case 2:
        old_value = shutdown_wearleveling_limit;
        break;
    case 3:
        old_value = elapsed(&shutdown_kRuntime);
        break;
    case 4:
        old_value = elapsed(&shutdown_started_at);
        break;
    case 5:
        old_value = elapsed(&shutdown_halt_chk_at);
        break;
    case 6:
        old_value = elapsed(&shutdown_wearleveling_done_at);
        break;

    default:
        break;
    }

    // swap the return value with the value that is in use
    i2cBuffer[2] =  ( (0xFF000000 & old_value) >>24 ); 
    i2cBuffer[3] =  ( (0x00FF0000 & old_value) >>16 ); 
    i2cBuffer[4] =  ( (0x0000FF00 & old_value) >>8 ); 
    i2cBuffer[5] =  ( (0x000000FF & old_value) ); 


    // keep the new_value and mark shutdown_limit_loaded to save in EEPROM
    // do not keep shutdown_[kRuntime|started_at|halt_chk_at|wearleveling_done_at]
    if (write)
    {
        switch (offset)
        {
        case 0:
            if (WriteEEShtDwnHaltTTL(&new_value))
            {
                shutdown_ttl_limit = new_value;
                shutdown_limit_loaded = HOSTSHUTDOWN_LIM_HALT_TTL_TOSAVE; // main loop will save to eeprom
            } 
            break;
        case 1:
            if (WriteEEShtDwnDelay(&new_value))
            {
                shutdown_delay_limit = new_value;
                shutdown_limit_loaded = HOSTSHUTDOWN_LIM_DELAY_TOSAVE; // main loop will save to eeprom
            } 
            break;
        case 2:
            if (WriteEEShtDwnWearleveling(&new_value))
            {
                shutdown_wearleveling_limit = new_value;
                shutdown_limit_loaded = HOSTSHUTDOWN_LIM_WEARLEVELING_TOSAVE; // main loop will save to eeprom
            } 
            break;

        default:
            break;
        }
    }
}

/********* PV and Battery Management ***********/

// I2C command to enable battery manager and set a i2c callback address for batmgr_state when command command byte is > zero.
// The manager operates as an i2c master and addresses the application MCU as a slave to update when events occur.
void fnBatteryMgr(uint8_t* i2cBuffer)
{ 
    enable_bm_callback_address = i2cBuffer[1]; // non-zero will turn on power manager and is the callback address used (the i2c slave address)
    battery_state_callback_cmd = i2cBuffer[2]; // callback will only happen if this value is > zero
}

// I2C command to access Battery charge low limit (int)
// battery manager will start charging with PWM mode when main power is bellow this limit
void fnBatChrgLowLim(uint8_t* i2cBuffer)
{
    // battery_low_limit is an int e.g., two bytes
    // save the new_value
    int new_value = 0;
    new_value += ((int)i2cBuffer[1])<<8;
    new_value += ((int)i2cBuffer[2]);

    // swap the return value
    i2cBuffer[1] =  ( (0xFF00 & battery_low_limit) >>8 ); 
    i2cBuffer[2] =  ( (0x00FF & battery_low_limit) ); 

    // keep the new_value and mark it to save in EEPROM
    if (IsValidBatLowLimFor12V(&new_value) || IsValidBatLowLimFor24V(&new_value))
    {
        battery_low_limit = new_value;
        bat_limit_loaded = BAT_LIM_LOW_TOSAVE; // main loop will save to eeprom
    }
}

// I2C command to access Battery charge high limit (int)
// battery manager will stop charging when main power is above this limit 
// PWM ontime is reduced as main power approches this limit
void fnBatChrgHighLim(uint8_t* i2cBuffer)
{
    // battery_high_limit is a int e.g., two bytes
    // save the new_value
    int new_value = 0;
    new_value += ((int)i2cBuffer[1])<<8;
    new_value += ((int)i2cBuffer[2]);

    // swap the return value
    i2cBuffer[1] =  ( (0xFF00 & battery_high_limit) >>8 ); 
    i2cBuffer[2] =  ( (0x00FF & battery_high_limit) ); 

    // keep the new_value and mark it to save in EEPROM
    if (IsValidBatHighLimFor12V(&new_value) || IsValidBatHighLimFor24V(&new_value))
    {
        battery_high_limit = new_value;
        bat_limit_loaded = BAT_LIM_HIGH_TOSAVE; // main loop will save to eeprom
    }
}

// I2C command to read battery charging time while doing pwm e.g., absorption time
void fnRdBatChrgTime(uint8_t* i2cBuffer)
{
    // there are four bytes in an unsigned long
    unsigned long my_copy = alt_pwm_accum_charge_time; // updates in ISR so copy first (when SMBus is done this is not used as an ISR callback)

    i2cBuffer[1] = ( (0xFF000000UL & my_copy) >>24 ); 
    i2cBuffer[2] = ( (0x00FF0000UL & my_copy) >>16 ); 
    i2cBuffer[3] = ( (0x0000FF00UL & my_copy) >>8 ); 
    i2cBuffer[4] = ( (0x000000FFUL & my_copy) );
}

// I2C command for day-night Morning Threshold (int)
void fnMorningThreshold(uint8_t* i2cBuffer)
{
    // daynight_morning_threshold is a uint16_t e.g., two bytes
    // save the new_value
    int new_value = 0;
    new_value += ((int)i2cBuffer[1])<<8;
    new_value += ((int)i2cBuffer[2]);

    // swap the return value
    i2cBuffer[1] =  ( (0xFF00 & daynight_morning_threshold) >>8 );
    i2cBuffer[2] =  ( (0x00FF & daynight_morning_threshold) ); 

    // keep the new_value and mark it to save in EEPROM
    if ( IsValidMorningThresholdFor12V(&new_value) || IsValidMorningThresholdFor24V(&new_value) )
    {
        daynight_morning_threshold = new_value;
        daynight_values_loaded = DAYNIGHT_MORNING_THRESHOLD_TOSAVE; // main loop will save to eeprom
    }
}

// I2C command for day-night Evening Threshold (int)
void fnEveningThreshold(uint8_t* i2cBuffer)
{
    // daynight_evening_threshold is a uint16_t e.g., two bytes
    // save the new_value
    int new_value = 0;
    new_value += ((int)i2cBuffer[1])<<8;
    new_value += ((int)i2cBuffer[2]);

    // swap the return value
    i2cBuffer[1] =  ( (0xFF00 & daynight_evening_threshold) >>8 ); 
    i2cBuffer[2] =  ( (0x00FF & daynight_evening_threshold) ); 

    // keep the new_value and mark it to save in EEPROM
    if ( (IsValidEveningThresholdFor12V(&new_value) || IsValidEveningThresholdFor24V(&new_value)) )
    {
        daynight_evening_threshold = new_value;
        daynight_values_loaded = DAYNIGHT_EVENING_THRESHOLD_TOSAVE; // main loop will save to eeprom
    }
}

// I2C command to set i2c callback address and three commands for daynight_state, day_work, and night_work events.
// The manager operates as an i2c master and addresses the application MCU as a slave to update when events occur.
void fnDayNightState(uint8_t* i2cBuffer)
{ 
    daynight_callback_address = i2cBuffer[1];
    daynight_state_callback_cmd = i2cBuffer[2];
    day_work_callback_cmd = i2cBuffer[3];
    night_work_callback_cmd = i2cBuffer[4];
}

/********* Analog ***********
  *  ADC_ENUM_t has ADC_ENUM_ALT_I, ADC_ENUM_ALT_V, ADC_ENUM_PWR_I, ADC_ENUM_PWR_V, ADC_ENUM_END
  *  and is used to refer to adc channels ADC_CH_ALT_I = 0, ADC_CH_ALT_V = 1, ADC_CH_PWR_I = 6, ADC_CH_PWR_V = 7
  * */

// I2C command to read the ADC_ENUM_t [0..3] value sent.
// returns the adc value with high byte after command byte, then low byte next.
// Most AVR have ten analog bits, thus range is: 0..1023
// returns zero when given an invalid channel
void fnAnalogRead(uint8_t* i2cBuffer)
{
    uint16_t adc_enum = 0;
    adc_enum += ((uint16_t)i2cBuffer[1])<<8;
    adc_enum += ((uint16_t)i2cBuffer[2]);
    uint16_t adc_reading;
    if ( (adc_enum < ADC_ENUM_END) )
    {
        adc_reading = adcAtomic(adcMap[adc_enum].channel);
    }
    else
    {
        adc_reading = 0; 
    }
    i2cBuffer[1] = ( (0xFF00 & adc_reading) >>8 ); 
    i2cBuffer[2] = ( (0x00FF & adc_reading) ); 
}

// I2C command for Calibration of ADC_CH_ALT_I, ADC_CH_ALT_V, ADC_CH_PWR_I, ADC_CH_PWR_V adc channels
// select the channel with ADC_ENUM_t value in byte after command
// swap the next four I2C buffer bytes with the calMap[convert_channel_to_cal_map_index(channel)].calibration float
// set cal_loaded so main loop will save it to EEPROM if valid or
// recover EEPROM (or default) value if new_value was not valid
void fnCalibrationRead(uint8_t* i2cBuffer)
{
    uint8_t is_adc_enum_with_writebit = i2cBuffer[1];
    uint8_t adc_enum  = is_adc_enum_with_writebit & CAL_CHANNEL_MASK; // removed the writebit
    if ( (adc_enum < ADC_ENUM_END) )
    {
        adc_enum_with_writebit = is_adc_enum_with_writebit;

        // place float in a uint32_t
        uint32_t old;
        float temp_calibration = calMap[adc_enum].calibration;
        memcpy(&old, &temp_calibration, sizeof old);

        uint32_t new_value = 0;
        new_value += ((uint32_t)i2cBuffer[2])<<24; // cast, multiply by 2**24, and sum 
        i2cBuffer[2] = ( (0xFF000000UL & old) >>24 ); // swap the return value with the old byte

        new_value += ((uint32_t)i2cBuffer[3])<<16;
        i2cBuffer[3] =  ( (0x00FF0000UL & old) >>16 ); 

        new_value += ((uint32_t)i2cBuffer[4])<<8;
        i2cBuffer[4] =  ( (0x0000FF00UL & old) >>8 ); 

        new_value += ((uint32_t)i2cBuffer[5]);
        i2cBuffer[5] =  ( (0x000000FFUL & old) ); 

        // new_value is ready
        if (is_adc_enum_with_writebit & CAL_CHANNEL_WRITEBIT) // keep new_value in SRAM if writebit is set
        {
            // copy bytes into the memory footprint used for our tempary float
            memcpy(&temp_calibration, &new_value, sizeof temp_calibration);
            calMap[adc_enum].calibration = temp_calibration;

            // CAL_0_TOSAVE << channelToCalMap[channel].cal_map 
            // if cal_map is 1 then CAL_0_TOSAVE << 1 gives CAL_1_TOSAVE 
            cal_loaded = CAL_0_TOSAVE << adc_enum; // main loop will save to eeprom or load default value if out of range
        }
    }
    else // bad channel  
    {
        i2cBuffer[1] = 0x7F;
        i2cBuffer[2] = 0xFF; // 0xFFFFFFFF is nan as a float
        i2cBuffer[3] = 0xFF;
        i2cBuffer[4] = 0xFF;
        i2cBuffer[5] = 0xFF;
    }
}

// I2C command to read timed accumulation/1e6 of analog input ALT_I or PWR_I
// ADC_ENUM_ALT_I == 0
// ADC_ENUM_PWR_I == 2
void fnRdTimedAccum(uint8_t* i2cBuffer)
{
    uint32_t adc_enum = 0;
    adc_enum += ((uint32_t)i2cBuffer[1])<<24;
    adc_enum += ((uint32_t)i2cBuffer[2])<<16;
    adc_enum += ((uint32_t)i2cBuffer[3])<<8;
    adc_enum += ((uint32_t)i2cBuffer[4]);
    unsigned long my_copy; //I2C runs this in ISR but durring SMBus this is not run in ISR context
    if (adc_enum == ADC_ENUM_ALT_I)
    {
        my_copy = accumulate_alt_mega_ti;
    }
    else if (adc_enum == ADC_ENUM_PWR_I)
    {
        my_copy = accumulate_pwr_mega_ti;
    }
    else
    {
        my_copy = 0; 
    }

    // there are four bytes in the unsigned long to send back
    i2cBuffer[1] = ( (0xFF000000UL & my_copy) >>24 ); 
    i2cBuffer[2] = ( (0x00FF0000UL & my_copy) >>16 ); 
    i2cBuffer[3] = ( (0x0000FF00UL & my_copy) >>8 ); 
    i2cBuffer[4] = ( (0x000000FFUL & my_copy) );
}

// I2C command for Referance EXTERNAL_AVCC and INTERN_1V1
// byte after command is used to select 
// swap the next four I2C buffer bytes with the refMap[select].reference float
// set ref_loaded so main loop will save it to EEPROM if valid or
// recover EEPROM (or default) value if new_value was not valid
void fnReferance(uint8_t* i2cBuffer)
{
    uint8_t is_referance_with_writebit = i2cBuffer[1];
    uint8_t referance = is_referance_with_writebit & REF_SELECT_MASK; // removed the writebit
    if ( (referance < REFERENCE_OPTIONS) )
    {
        ref_select_with_writebit = is_referance_with_writebit;

        // place old float in a uint32_t
        uint32_t old;
        float temp_referance = refMap[referance].reference;
        memcpy(&old, &temp_referance, sizeof old);

        uint32_t new_value = 0;
        new_value += ((uint32_t)i2cBuffer[2])<<24; // cast, multiply by 2**24, and sum 
        i2cBuffer[2] = ( (0xFF000000UL & old) >>24 ); // swap the return value with the old byte

        new_value += ((uint32_t)i2cBuffer[3])<<16;
        i2cBuffer[3] =  ( (0x00FF0000UL & old) >>16 ); 

        new_value += ((uint32_t)i2cBuffer[4])<<8;
        i2cBuffer[4] =  ( (0x0000FF00UL & old) >>8 ); 

        new_value += ((uint32_t)i2cBuffer[5]);
        i2cBuffer[5] =  ( (0x000000FFUL & old) ); 

        // new_value is ready
        if (is_referance_with_writebit & REF_SELECT_WRITEBIT) // keep new_value in SRAM if writebit is set
        {
            // copy bytes into the memory footprint used for our tempary float
            memcpy(&temp_referance, &new_value, sizeof temp_referance);
            refMap[referance].reference = temp_referance;

            // if referance is 1 then REF_0_TOSAVE << 1 gives REF_1_TOSAVE 
            ref_loaded = REF_0_TOSAVE << referance; // main loop will save to eeprom or load default value if bad value
        }
    }
    else // bad select  
    {
        i2cBuffer[1] = 0x7F;
        i2cBuffer[2] = 0xFF; // 0xFFFFFFFF is nan as a float
        i2cBuffer[3] = 0xFF;
        i2cBuffer[4] = 0xFF;
        i2cBuffer[5] = 0xFF;
    }
}

/********* TEST MODE ***********
  *    trancever control for testing      */

// I2C command to start test_mode
void fnStartTestMode(uint8_t* i2cBuffer)
{
    if (i2cBuffer[1] == 1)
    {
        if (!test_mode_started && !test_mode)
        {
            uart_started_at = milliseconds();
            uart_output = RPU_START_TEST_MODE;
            printf("%c%c", uart_output, ( (~uart_output & 0x0A) << 4 | (~uart_output & 0x50) >> 4 ) ); 
            uart_has_TTL = 1; // causes host_is_foreign to be false
            test_mode_started = 1; // it is cleared by check_uart where test_mode is set
        } 
        else
        {
            i2cBuffer[1] = 2; // repeated commands are ignored until check_uart is done
        }
    }
    else 
    {
        // read the local address to send a byte on DTR for RPU_NORMAL_MODE
        i2cBuffer[1] = 3; // ignore everything but the command
    }
}

// I2C command to end test_mode
void fnEndTestMode(uint8_t* i2cBuffer)
{
    if (i2cBuffer[1] == 1)
    {
        if (!test_mode_started && test_mode)
        {
            ioWrite(MCU_IO_DTR_TXD, LOGIC_LEVEL_HIGH); // strong pullup
            ioDir(MCU_IO_DTR_TXD, DIRECTION_INPUT); // the DTR pair driver will see a weak pullup when UART starts
            stdout = stdin = uart0_init(DTR_BAUD,UART0_RX_REPLACE_CR_WITH_NL); // turn on UART
            ioWrite(MCU_IO_DTR_DE, LOGIC_LEVEL_HIGH); //DTR transceiver may have been turned off during the test
            ioWrite(MCU_IO_DTR_nRE, LOGIC_LEVEL_LOW); 
            uart_started_at = milliseconds();
            uart_output = RPU_END_TEST_MODE;
            printf("%c%c", uart_output, ( (~uart_output & 0x0A) << 4 | (~uart_output & 0x50) >> 4 ) ); 
            uart_has_TTL = 1; // causes host_is_foreign to be false
            test_mode_started = 1; // it is cleared by check_uart where test_mode is also cleared
            i2cBuffer[1] = transceiver_state; // replace the data byte with the transceiver_state.
        } 
        else
        {
            i2cBuffer[1] = 0; // repeated commands are ignored until check_uart is done
        }
    }
    else 
    {
        // read the local address to send a byte on DTR for RPU_NORMAL_MODE
        i2cBuffer[1] = 0; // ignore everything but the command
    }
}

// I2C command to read transceiver control bits
void fnRdXcvrCntlInTestMode(uint8_t* i2cBuffer)
{
    if (test_mode)
    {
        i2cBuffer[1] = ( (ioRead(MCU_IO_HOST_nRTS)<<7) | (ioRead(MCU_IO_HOST_nCTS)<<6) | \
                         (ioRead(MCU_IO_TX_nRE)<<5) | (ioRead(MCU_IO_TX_DE)<<4) | \
                         (ioRead(MCU_IO_DTR_nRE)<<3) | (ioRead(MCU_IO_DTR_DE)<<2) | \
                         (ioRead(MCU_IO_RX_nRE)<<1) | (ioRead(MCU_IO_RX_DE)) ); 
    }
    else 
    {
        i2cBuffer[1] = 0; 
    }
}

// I2C command to write transceiver control bits
void fnWtXcvrCntlInTestMode(uint8_t* i2cBuffer)
{
    if (test_mode)
    {
        // mask the needed bit and shift it to position zero so it can be cast for ioWrite.
        ioWrite(MCU_IO_HOST_nRTS, (LOGIC_LEVEL_t) ( (i2cBuffer[1] & (1<<7))>>7 ) );
        ioWrite(MCU_IO_HOST_nCTS,  (LOGIC_LEVEL_t) ( (i2cBuffer[1] & (1<<6))>>6 ) );
        ioWrite(MCU_IO_TX_nRE,  (LOGIC_LEVEL_t) ( (i2cBuffer[1] & (1<<5))>>5 ) );
        ioWrite(MCU_IO_TX_DE,  (LOGIC_LEVEL_t) ( (i2cBuffer[1] & (1<<4))>>4 ) );
        ioWrite(MCU_IO_DTR_nRE,  (LOGIC_LEVEL_t) ( (i2cBuffer[1] & (1<<3))>>3 ) ); // setting this will blind others to multi-drop bus state change but is needed for testing
        if ( (i2cBuffer[1] & (1<<2))>>2 ) // enabling the dtr driver in testmode needs to cause the transcever to power the dtr pair to check for a 50 Ohm load
        {
            stdout = stdin = uart0_init(0,0); // turn off DTR UART 
            ioDir(MCU_IO_DTR_TXD, DIRECTION_OUTPUT);
            ioWrite(MCU_IO_DTR_TXD,LOGIC_LEVEL_LOW); // the DTR pair will be driven and load the transceiver 
            ioWrite(MCU_IO_DTR_DE, LOGIC_LEVEL_HIGH); 
        }
        ioWrite(MCU_IO_RX_nRE, (LOGIC_LEVEL_t) ( (i2cBuffer[1] & (1<<1))>>1 ) );
        ioWrite(MCU_IO_RX_DE, (LOGIC_LEVEL_t)  (i2cBuffer[1] & 1) );
    }
    else 
    {
        i2cBuffer[1] = 0; 
    }
}

// I2C command for day-night morning debounce time (unsigned long)
void fnMorningDebounce(uint8_t* i2cBuffer)
{
    // daynight_morning_debounce is a unsigned long and has four bytes
    // save the new_value
    uint32_t new_value = 0;
    new_value += ((uint32_t)i2cBuffer[1])<<24; // cast, multiply by 2**24, and sum 
    new_value += ((uint32_t)i2cBuffer[2])<<16;
    new_value += ((uint32_t)i2cBuffer[3])<<8;
    new_value += ((uint32_t)i2cBuffer[4]);

    // swap the return value
    i2cBuffer[1] = ( (0xFF000000UL & daynight_morning_debounce) >>24 ); 
    i2cBuffer[2] =  ( (0x00FF0000UL & daynight_morning_debounce) >>16 ); 
    i2cBuffer[3] =  ( (0x0000FF00UL & daynight_morning_debounce) >>8 ); 
    i2cBuffer[4] =  ( (0x000000FFUL & daynight_morning_debounce) ); 

    // keep the new_value and mark it to save in EEPROM
    if (IsValidMorningDebounce(&new_value))
    {
        daynight_morning_debounce = new_value;
        daynight_values_loaded = DAYNIGHT_MORNING_DEBOUNCE_TOSAVE; // main loop will save to eeprom
    }
}

// I2C command for day-night evening debounce time (unsigned long)
void fnEveningDebounce(uint8_t* i2cBuffer)
{
    // daynight_evening_debounce is a unsigned long and has four bytes
    // save the new_value
    uint32_t new_value = 0;
    new_value += ((uint32_t)i2cBuffer[1])<<24; // cast, multiply by 2**24, and sum 
    new_value += ((uint32_t)i2cBuffer[2])<<16;
    new_value += ((uint32_t)i2cBuffer[3])<<8;
    new_value += ((uint32_t)i2cBuffer[4]);

    // swap the return value
    i2cBuffer[1] = ( (0xFF000000UL & daynight_evening_debounce) >>24 ); // swap the return value with the old byte
    i2cBuffer[2] =  ( (0x00FF0000UL & daynight_evening_debounce) >>16 ); 
    i2cBuffer[3] =  ( (0x0000FF00UL & daynight_evening_debounce) >>8 ); 
    i2cBuffer[4] =  ( (0x000000FFUL & daynight_evening_debounce) ); 

    // keep the new_value and mark it to save in EEPROM
    if (IsValidEveningDebounce(&new_value))
    {
        daynight_evening_debounce = new_value;
        daynight_values_loaded = DAYNIGHT_EVENING_DEBOUNCE_TOSAVE; // main loop will save to eeprom
    }
}

// I2C command to read elapsed_time_since_dayTmrStarted
void fnDayNightTimer(uint8_t* i2cBuffer)
{
    unsigned long elapsed_time_since_dayTmrStarted = elapsed(&dayTmrStarted);
    // there are four bytes in an unsigned long
    i2cBuffer[1] = ( (0xFF000000UL & elapsed_time_since_dayTmrStarted) >>24 ); 
    i2cBuffer[2] =  ( (0x00FF0000UL & elapsed_time_since_dayTmrStarted) >>16 ); 
    i2cBuffer[3] =  ( (0x0000FF00UL & elapsed_time_since_dayTmrStarted) >>8 ); 
    i2cBuffer[4] =  ( (0x000000FFUL & elapsed_time_since_dayTmrStarted) );
}

/* Dummy function */
void fnNull(uint8_t* i2cBuffer)
{
    return; 
}