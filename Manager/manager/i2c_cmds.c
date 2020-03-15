/*
i2c_cmds is a i2c RPUBUS manager commands library in the form of a function pointer array  
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
#include "power_manager.h"
#include "battery_limits.h"
#include "daynight_limits.h"
#include "daynight_state.h"
#include "calibration_limits.h"

uint8_t i2c0Buffer[I2C_BUFFER_LENGTH];
uint8_t i2c0BufferLength = 0;

// called when I2C data is received. 
void receive_i2c_event(uint8_t* inBytes, uint8_t numBytes) 
{
    // table of pointers to functions that are selected by the i2c cmmand byte
    static void (*pf[GROUP][MGR_CMDS])(uint8_t*) = 
    {
        {fnRdMgrAddr, fnNull, fnRdBootldAddr, fnWtBootldAddr, fnRdShtdnDtct, fnWtShtdnDtct, fnRdStatus, fnWtStatus},
        {fnWtArduinMode, fnRdArduinMode, fnBatStartChrg, fnBatDoneChrg, fnRdBatChrgTime, fnMorningThreshold, fnEveningThreshold, fnDayNightState},
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
void fnRdMgrAddr(uint8_t* i2cBuffer)
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
void fnRdMgrAddrQuietly(uint8_t* i2cBuffer)
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

// I2C_COMMAND_TO_READ_ADDRESS_SENT_ON_ACTIVE_DTR
void fnRdBootldAddr(uint8_t* i2cBuffer)
{
    // replace data[1] with address sent when HOST_nRTS toggles
    i2cBuffer[1] = bootloader_address;
}

// I2C_COMMAND_TO_SET_ADDRESS_SENT_ON_ACTIVE_DTR
void fnWtBootldAddr(uint8_t* i2cBuffer)
{
    // set the byte that is sent when HOST_nRTS toggles
    bootloader_address = i2cBuffer[1];
}

// I2C_COMMAND_TO_READ_SW_SHUTDOWN_DETECTED
void fnRdShtdnDtct(uint8_t* i2cBuffer)
{
    // when ICP1 pin is pulled  down the host (e.g. R-Pi Zero) should be set up to hault
    i2cBuffer[1] = shutdown_detected;
    // reading clears this flag that was set in check_shutdown() but it is up to the I2C master to do somthing about it.
    shutdown_detected = 0;
}

// I2C_COMMAND_TO_SET_SW_FOR_SHUTDOWN
void fnWtShtdnDtct(uint8_t* i2cBuffer)
{
    // pull ICP1 pin low to hault the host (e.g. Pi Zero on RPUpi)
    if (i2cBuffer[1] == 1)
    {
        ioDir(MCU_IO_SHUTDOWN, DIRECTION_OUTPUT);
        ioWrite(MCU_IO_SHUTDOWN, LOGIC_LEVEL_LOW);
        ioDir(MCU_IO_MGR_SCK_LED, DIRECTION_OUTPUT);
        ioWrite(MCU_IO_MGR_SCK_LED, LOGIC_LEVEL_HIGH);
        shutdown_started = 1; // it is cleared in check_shutdown()
        shutdown_detected = 0; // it is set in check_shutdown()
        shutdown_started_at = milliseconds();
    }
    // else ignore
}

// I2C_COMMAND_TO_READ_STATUS
void fnRdStatus(uint8_t* i2cBuffer)
{
    i2cBuffer[1] = status_byt & 0x0F; // bits 0..3
    if (ioRead(MCU_IO_ALT_EN)) 
        i2cBuffer[1] += (1<<4); // include bit 4 if alternat power is enabled
    if (ioRead(MCU_IO_PIPWR_EN)) 
        i2cBuffer[1] += (1<<5); // include bit 5 if sbc has power
    if (daynight_state==DAYNIGHT_FAIL_STATE) i2cBuffer[1] += (1<<6); //  include bit 6 if daynight state has failed
}

// I2C_COMMAND_TO_SET_STATUS
void fnWtStatus(uint8_t* i2cBuffer)
{
    if ( (i2cBuffer[1] & 0x10) ) 
    {
        enable_alternate_power = 1;
        alt_pwm_accum_charge_time = 0; // clear charge time
    }
    if ( ( i2cBuffer[1] & (1<<5) ) && !shutdown_started && !shutdown_detected )
    {
        ioWrite(MCU_IO_PIPWR_EN,LOGIC_LEVEL_HIGH); //restart SBC 
    } 
    if ( ( i2cBuffer[1] & (1<<6) ) ) daynight_state = DAYNIGHT_START_STATE; // restart the state machine
    status_byt = i2cBuffer[1] & 0x0F; // set bits 0..3
}

/********* PIONT TO POINT MODE ***********
  *    arduino_mode LOCKOUT_DELAY and BOOTLOADER_ACTIVE last forever when the host RTS toggles   */

// I2C command to set arduino_mode
void fnWtArduinMode(uint8_t* i2cBuffer)
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

// I2C command to read arduino_mode
void fnRdArduinMode(uint8_t* i2cBuffer)
{
    i2cBuffer[1] = arduino_mode;
}

// I2C command for Battery charge start limit (uint16_t)
void fnBatStartChrg(uint8_t* i2cBuffer)
{
    // battery_low_limit is a uint16_t e.g., two bytes
    uint16_t old = battery_low_limit;
    uint16_t new = 0;

    new += ((uint16_t)i2cBuffer[1])<<8;
    i2cBuffer[1] =  ( (0xFF00 & old) >>8 ); 

    new += ((uint16_t)i2cBuffer[2]);
    i2cBuffer[2] =  ( (0x00FF & old) ); 

    // new is ready
    battery_low_limit = new;

    bat_limit_loaded = BAT_LOW_LIM_TOSAVE; // main loop will save to eeprom or load default value if new value is out of range
}

// I2C command for Battery charge done limit (uint16_t)
void fnBatDoneChrg(uint8_t* i2cBuffer)
{
    // battery_high_limit is a uint16_t e.g., two bytes
    uint16_t old = battery_high_limit;
    uint16_t new = 0;

    new += ((uint16_t)i2cBuffer[1])<<8;
    i2cBuffer[1] =  ( (0xFF00 & old) >>8 ); 

    new += ((uint16_t)i2cBuffer[2]);
    i2cBuffer[2] =  ( (0x00FF & old) ); 

    // new is ready
    battery_high_limit = new;

    bat_limit_loaded = BAT_HIGH_LIM_TOSAVE; // main loop will save to eeprom or load default value if new value is out of range
}

// I2C command to read battery charging time while doing pwm e.g., absorption time
void fnRdBatChrgTime(uint8_t* i2cBuffer)
{
    // there are four bytes in an unsigned long
    unsigned long my_copy = alt_pwm_accum_charge_time; //updates in ISR so copy first (when SMBus is done this is not used as an ISR callback)

    i2cBuffer[1] = ( (0xFF000000UL & my_copy) >>24 ); 
    i2cBuffer[2] = ( (0x00FF0000UL & my_copy) >>16 ); 
    i2cBuffer[3] = ( (0x0000FF00UL & my_copy) >>8 ); 
    i2cBuffer[4] = ( (0x000000FFUL & my_copy) );
}

// I2C command for day-night Morning Threshold (uint16_t)
void fnMorningThreshold(uint8_t* i2cBuffer)
{
    // daynight_morning_threshold is a uint16_t e.g., two bytes
    uint16_t old = daynight_morning_threshold;
    uint16_t new = 0;

    new += ((uint16_t)i2cBuffer[1])<<8;
    i2cBuffer[1] =  ( (0xFF00 & old) >>8 ); 

    new += ((uint16_t)i2cBuffer[2]);
    i2cBuffer[2] =  ( (0x00FF & old) ); 

    // new is ready
    daynight_morning_threshold = new;
    
    daynight_values_loaded = DAYNIGHT_MORNING_THRESHOLD_TOSAVE; // main loop will save to eeprom or load default value if new value is out of range
}

// I2C command for day-night Evening Threshold (uint16_t)
void fnEveningThreshold(uint8_t* i2cBuffer)
{
    // daynight_evening_threshold is a uint16_t e.g., two bytes
    uint16_t old = daynight_evening_threshold;
    uint16_t new = 0;

    new += ((uint16_t)i2cBuffer[1])<<8;
    i2cBuffer[1] =  ( (0xFF00 & old) >>8 ); 

    new += ((uint16_t)i2cBuffer[2]);
    i2cBuffer[2] =  ( (0x00FF & old) ); 

    // new is ready
    daynight_evening_threshold = new;

    daynight_values_loaded = DAYNIGHT_EVENING_THRESHOLD_TOSAVE; // main loop will save to eeprom or load default value if new value is out of range
}

// I2C command to read Day-Night state. It is one byte,
// the low nimmble has daynight_state, the high nimmble has daynight_work
// bit 4 set from master will clear daynight_work (readback depends on if bit 5 is set)
// bit 5 set from master will include daynight_work otherwise only daynight_state is in readback.
void fnDayNightState(uint8_t* i2cBuffer)
{ 
    if (i2cBuffer[1] & (1<<4) )
    {
        daynight_work = 0;  // clear daynight_work
    }
    if (i2cBuffer[1] & (1<<5) ) 
    {
        i2cBuffer[1] = daynight_state + daynight_work;  // send back daynight_state + daynignt_work
    }
    else
    {
        i2cBuffer[1] = daynight_state;  // send back only daynight_state
    }
}

/********* POWER MANAGER ***********
  *  analogRead of ALT_I, ALT_V, PWR_I, PWR_V reading     
  *  analogTimedAccumulationRead of ALT_I, PWR_I */

// I2C command to read the analog channel sent.
// returns analogRead with high byte after command byte, then low byte next.
// Most AVR have ten analog bits, thus range is: 0..1023
// returns zero when given an invalid channel
void fnAnalogRead(uint8_t* i2cBuffer)
{
    uint16_t channel = 0;
    channel += ((uint16_t)i2cBuffer[1])<<8;
    channel += ((uint16_t)i2cBuffer[2]);
    uint16_t adc_reading;
    if ( (channel == ADC_CH_ALT_I) || (channel == ADC_CH_ALT_V) || \
         (channel == ADC_CH_PWR_I) || (channel == ADC_CH_PWR_V) )
    {
        adc_reading = adcAtomic((uint8_t)channel);
    }
    else
    {
        adc_reading = 0; 
    }
    i2cBuffer[1] = ( (0xFF00 & adc_reading) >>8 ); 
    i2cBuffer[2] = ( (0x00FF & adc_reading) ); 
}

// I2C command for Calibration of ADC_CH_ALT_I, ADC_CH_ALT_V, ADC_CH_PWR_I, ADC_CH_PWR_V adc channels
// byte after command is used to select channel
// swap the next four I2C buffer bytes with the calMap[convert_channel_to_cal_map_index(channel)].calibration float
// set cal_loaded so main loop will save it to EEPROM if valid or
// recover EEPROM (or default) value if new was not valid
void fnCalibrationRead(uint8_t* i2cBuffer)
{
    uint8_t is_channel_with_writebit = i2cBuffer[1];
    uint8_t channel  = is_channel_with_writebit & CAL_CHANNEL_MASK; // removed the writebit
    if ( (channel == ADC_CH_ALT_I) || (channel == ADC_CH_ALT_V) || \
         (channel == ADC_CH_PWR_I) || (channel == ADC_CH_PWR_V) )
    {
        channel_with_writebit = is_channel_with_writebit;

        // place float in a uint32_t
        uint32_t old;
        float temp_calibration = calMap[channelMap[channel].cal_map].calibration;
        memcpy(&old, &temp_calibration, sizeof old);

        uint32_t new = 0;
        new += ((uint32_t)i2cBuffer[2])<<24; // cast, multiply by 2**24, and sum 
        i2cBuffer[2] = ( (0xFF000000UL & old) >>24 ); // swap the return value with the old byte

        new += ((uint32_t)i2cBuffer[3])<<16;
        i2cBuffer[3] =  ( (0x00FF0000UL & old) >>16 ); 

        new += ((uint32_t)i2cBuffer[4])<<8;
        i2cBuffer[4] =  ( (0x0000FF00UL & old) >>8 ); 

        new += ((uint32_t)i2cBuffer[5]);
        i2cBuffer[5] =  ( (0x000000FFUL & old) ); 

        // new is ready
        if (is_channel_with_writebit & CAL_CHANNEL_WRITEBIT) // keep new in SRAM if writebit is set
        {
            // copy bytes into the memory footprint used for our tempary float
            memcpy(&temp_calibration, &new, sizeof temp_calibration);
            calMap[channelMap[channel].cal_map].calibration = temp_calibration;

            // CAL_0_TOSAVE << channelMap[channel].cal_map 
            // if cal_map is 1 then CAL_0_TOSAVE << 1 gives CAL_1_TOSAVE 
            cal_loaded = CAL_0_TOSAVE << channelMap[channel].cal_map; // main loop will save to eeprom or load default value if out of range
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

// I2C command to read timed accumulation of analog channel ALT_I or PWR_I sent
void fnRdTimedAccum(uint8_t* i2cBuffer)
{
    uint32_t channel = 0;
    channel += ((uint32_t)i2cBuffer[1])<<24;
    channel += ((uint32_t)i2cBuffer[2])<<16;
    channel += ((uint32_t)i2cBuffer[3])<<8;
    channel += ((uint32_t)i2cBuffer[4]);
    unsigned long my_copy; //I2C runs this in ISR but durring SMBus this is not run in ISR context
    if (channel == ADC_CH_ALT_I)
    {
        my_copy = accumulate_alt_ti;
    }
    else if (channel == ADC_CH_PWR_I)
    {
        my_copy = accumulate_pwr_ti;
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
// recover EEPROM (or default) value if new was not valid
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

        uint32_t new = 0;
        new += ((uint32_t)i2cBuffer[2])<<24; // cast, multiply by 2**24, and sum 
        i2cBuffer[2] = ( (0xFF000000UL & old) >>24 ); // swap the return value with the old byte

        new += ((uint32_t)i2cBuffer[3])<<16;
        i2cBuffer[3] =  ( (0x00FF0000UL & old) >>16 ); 

        new += ((uint32_t)i2cBuffer[4])<<8;
        i2cBuffer[4] =  ( (0x0000FF00UL & old) >>8 ); 

        new += ((uint32_t)i2cBuffer[5]);
        i2cBuffer[5] =  ( (0x000000FFUL & old) ); 

        // new is ready
        if (is_referance_with_writebit & REF_SELECT_WRITEBIT) // keep new in SRAM if writebit is set
        {
            // copy bytes into the memory footprint used for our tempary float
            memcpy(&temp_referance, &new, sizeof temp_referance);
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
    uint32_t old = daynight_morning_debounce;
    uint32_t new = 0;
    new += ((uint32_t)i2cBuffer[1])<<24; // cast, multiply by 2**24, and sum 
    i2cBuffer[1] = ( (0xFF000000UL & old) >>24 ); // swap the return value with the old byte

    new += ((uint32_t)i2cBuffer[2])<<16;
    i2cBuffer[2] =  ( (0x00FF0000UL & old) >>16 ); 

    new += ((uint32_t)i2cBuffer[3])<<8;
    i2cBuffer[3] =  ( (0x0000FF00UL & old) >>8 ); 

    new += ((uint32_t)i2cBuffer[4]);
    i2cBuffer[4] =  ( (0x000000FFUL & old) ); 

    // new is ready
    daynight_morning_debounce = new;

    daynight_values_loaded = DAYNIGHT_MORNING_DEBOUNCE_TOSAVE; // main loop will save to eeprom or load default value if new value is out of range
}

// I2C command for day-night evening debounce time (unsigned long)
void fnEveningDebounce(uint8_t* i2cBuffer)
{
    // daynight_evening_debounce is a unsigned long and has four bytes
    uint32_t old = daynight_evening_debounce;
    uint32_t new = 0;
    new += ((uint32_t)i2cBuffer[1])<<24; // cast, multiply by 2**24, and sum 
    i2cBuffer[1] = ( (0xFF000000UL & old) >>24 ); // swap the return value with the old byte

    new += ((uint32_t)i2cBuffer[2])<<16;
    i2cBuffer[2] =  ( (0x00FF0000UL & old) >>16 ); 

    new += ((uint32_t)i2cBuffer[3])<<8;
    i2cBuffer[3] =  ( (0x0000FF00UL & old) >>8 ); 

    new += ((uint32_t)i2cBuffer[4]);
    i2cBuffer[4] =  ( (0x000000FFUL & old) ); 

    // new is ready
    daynight_evening_debounce = new;

    daynight_values_loaded = DAYNIGHT_EVENING_DEBOUNCE_TOSAVE; // main loop will save to eeprom or load default value if new value is out of range
}

// I2C command to read day daynight timer offset
void fnDayNightTimer(uint8_t* i2cBuffer)
{
    unsigned long daynight_timer = elapsed(&dayTmrStarted);
    // there are four bytes in an unsigned long
    i2cBuffer[1] = ( (0xFF000000UL & daynight_timer) >>24 ); 
    i2cBuffer[2] =  ( (0x00FF0000UL & daynight_timer) >>16 ); 
    i2cBuffer[3] =  ( (0x0000FF00UL & daynight_timer) >>8 ); 
    i2cBuffer[4] =  ( (0x000000FFUL & daynight_timer) );
}

/* Dummy function */
void fnNull(uint8_t* i2cBuffer)
{
    return; 
}