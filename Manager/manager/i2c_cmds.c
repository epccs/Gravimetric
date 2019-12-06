/*
i2c_cmds is a i2c RPUBUS manager commands library in the form of a function pointer array  
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

#include <avr/io.h>
#include "../lib/timers.h"
#include "../lib/twi0.h"
#include "../lib/uart.h"
#include "../lib/adc.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"
#include "rpubus_manager_state.h"
#include "dtr_transmition.h"
#include "i2c_cmds.h"
#include "adc_burst.h"
#include "references.h"
#include "power_manager.h"
#include "battery_limits.h"
#include "daynight_limits.h"
#include "daynight_state.h"

uint8_t i2c0Buffer[I2C_BUFFER_LENGTH];
uint8_t i2c0BufferLength = 0;

// called when I2C data is received. 
void receive_i2c_event(uint8_t* inBytes, int numBytes) 
{
    // table of pointers to functions that are selected by the i2c cmmand byte
    static void (*pf[GROUP][MGR_CMDS])(uint8_t*) = 
    {
        {fnRdMgrAddr, fnWtMgrAddr, fnRdBootldAddr, fnWtBootldAddr, fnRdShtdnDtct, fnWtShtdnDtct, fnRdStatus, fnWtStatus},
        {fnWtArduinMode, fnRdArduinMode, fnBatStartChrg, fnBatDoneChrg, fnRdBatChrgTime, fnMorningThreshold, fnEveningThreshold, fnDayNightState},
        {fnRdAdcAltI, fnRdAdcAltV, fnRdAdcPwrI, fnRdAdcPwrV, fnRdTimedAccumAltI, fnRdTimedAccumPwrI, fnAnalogRefExternAVCC, fnAnalogRefIntern1V1},
        {fnStartTestMode, fnEndTestMode, fnRdXcvrCntlInTestMode, fnWtXcvrCntlInTestMode, fnMorningDebounce, fnEveningDebounce, fnMillis, fnNull}
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
    uint8_t return_code = twi0_transmit(i2c0Buffer, i2c0BufferLength);
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
        uart_started_at = millis();
        uart_output = RPU_NORMAL_MODE;
        printf("%c%c", uart_output, ( (~uart_output & 0x0A) << 4 | (~uart_output & 0x50) >> 4 ) ); 
        uart_has_TTL = 1; // causes host_is_foreign to be false
    }
    else 
        if (bootloader_started)
        {
            // If the bootloader_started has not timed out yet broadcast on DTR pair
            uart_started_at = millis();
            uart_output = RPU_NORMAL_MODE;
            printf("%c%c", uart_output, ( (~uart_output & 0x0A) << 4 | (~uart_output & 0x50) >> 4 ) ); 
            uart_has_TTL = 0; // causes host_is_foreign to be true, so local DTR/RTS is not accepted
        } 
        else
        {
            lockout_started_at = millis() - LOCKOUT_DELAY;
            bootloader_started_at = millis() - BOOTLOADER_ACTIVE;
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

// (Obsolete) I2C command to access manager address
void fnWtMgrAddr(uint8_t* i2cBuffer)
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
        pinMode(SHUTDOWN, OUTPUT);
        digitalWrite(SHUTDOWN, LOW);
        pinMode(LED_BUILTIN, OUTPUT);
        digitalWrite(LED_BUILTIN, HIGH);
        shutdown_started = 1; // it is cleared in check_shutdown()
        shutdown_detected = 0; // it is set in check_shutdown()
        shutdown_started_at = millis();
    }
    // else ignore
}

// I2C_COMMAND_TO_READ_STATUS
void fnRdStatus(uint8_t* i2cBuffer)
{
    i2cBuffer[1] = status_byt & 0x0F; // bits 0..3
    i2cBuffer[1] &= digitalRead(ALT_EN)<<4; // report if alternat power is enabled
    i2cBuffer[1] &= digitalRead(PIPWR_EN)<<5; // report if sbc has power
}

// I2C_COMMAND_TO_SET_STATUS
void fnWtStatus(uint8_t* i2cBuffer)
{
    if ( (i2cBuffer[1] & 0x10) ) 
    {
        enable_alternate_power = 1;
        alt_pwm_accum_charge_time = 0; // clear charge time
    }
    if ( (i2cBuffer[1] & 0x20) && !shutdown_started && !shutdown_detected ) enable_sbc_power = 1;
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
            uart_started_at = millis();
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
    uint8_t temp = (battery_low_limit>>8) & 0xFF;
    battery_low_limit = 0x00FF & battery_low_limit; // mask out the old value
    battery_low_limit = ((uint32_t) (i2cBuffer[1])<<8) & battery_low_limit; // place new value in high byte
    i2cBuffer[1] = temp; // swap the return value with the old high byte

    temp = battery_low_limit & 0xFF;
    battery_low_limit = 0xFFFFFF00 & battery_low_limit;
    battery_low_limit = ((uint32_t) (i2cBuffer[2])) & battery_low_limit;  
    i2cBuffer[2] = temp;
    
    bat_limit_loaded = BAT_LOW_LIM_TOSAVE; // main loop will save to eeprom or load default value if new value is out of range
}

// I2C command for Battery charge done limit (uint16_t)
void fnBatDoneChrg(uint8_t* i2cBuffer)
{
    // battery_high_limit is a uint16_t e.g., two bytes
    uint8_t temp = (battery_high_limit>>8) & 0xFF;
    battery_high_limit = 0x00FF & battery_high_limit; // mask out the old value
    battery_high_limit = ((uint32_t) (i2cBuffer[1])<<8) & battery_high_limit; // place new value in high byte
    i2cBuffer[1] = temp; // swap the return value with the old high byte

    temp = battery_high_limit & 0xFF;
    battery_high_limit = 0xFFFFFF00 & battery_high_limit;
    battery_high_limit = ((uint32_t) (i2cBuffer[2])) & battery_high_limit;  
    i2cBuffer[2] = temp;
    
    bat_limit_loaded = BAT_LOW_LIM_TOSAVE; // main loop will save to eeprom or load default value if new value is out of range
}

// I2C command to read battery charging time while doing pwm e.g., absorption time
void fnRdBatChrgTime(uint8_t* i2cBuffer)
{
    // there are four bytes in an unsigned long
    i2cBuffer[1] =  (alt_pwm_accum_charge_time>>24) & 0xFF; // high byte. Mask is for clarity, the compiler should optimize it out
    i2cBuffer[2] =  (alt_pwm_accum_charge_time>>16) & 0xFF;
    i2cBuffer[3] =  (alt_pwm_accum_charge_time>>8) & 0xFF;
    i2cBuffer[4] =  alt_pwm_accum_charge_time & 0xFF; // low byte. Again Mask should optimize out
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
// the low nimmble has Day-Night state, the high nimmble helps tell if start of day (bit 6) or night (bit 7)
// bit 4 set from master will clear bits 6 & 7 (readback depends on if bit 5 is set)
// bit 5 set from master will include bits 6 & 7 in readback otherwise only only the Day-Night state is in readback
void fnDayNightState(uint8_t* i2cBuffer)
{ 
    if (i2cBuffer[1] & (1<<4) )
    {
        daynight_state &= ~( (1<<6) | (1<<7) );  // clear the day and night work bits
    }
    if (i2cBuffer[1] & (1<<5) ) 
    {
        i2cBuffer[1] = daynight_state & ~( (1<<4) | (1<<5) );  // return bits mask is 0b11001111
    }
    else
    {
        i2cBuffer[1] = daynight_state & ~( (1<<7) | (1<<6) | (1<<5) | (1<<4) );  // return bits mask is 0b00001111
    }
}

/********* POWER MANAGER ***********
  *  for ALT_I, ALT_V, PWR_I, PWR_V reading     */

// I2C command to read analog channel 0
void fnRdAdcAltI(uint8_t* i2cBuffer)
{
    uint16_t adc_buffer = analogRead(ALT_I);
    i2cBuffer[1] =  (adc_buffer>>8) & 0xFF; // high byte. Mask is for clarity, the compiler should optimize it out
    i2cBuffer[2] =  adc_buffer & 0xFF; // low byte. Again Mask should optimize out
}

// I2C command to read analog channel 1
void fnRdAdcAltV(uint8_t* i2cBuffer)
{
    uint16_t adc_buffer = analogRead(ALT_V);
    i2cBuffer[1] =  (adc_buffer>>8) & 0xFF;
    i2cBuffer[2] =  adc_buffer & 0xFF;
}

// I2C command to read analog channel 6
void fnRdAdcPwrI(uint8_t* i2cBuffer)
{
    uint16_t adc_buffer = analogRead(PWR_I);
    i2cBuffer[1] =  (adc_buffer>>8) & 0xFF;
    i2cBuffer[2] =  adc_buffer & 0xFF;
}

// I2C command to read analog channel 7
void fnRdAdcPwrV(uint8_t* i2cBuffer)
{
    uint16_t adc_buffer = analogRead(PWR_V);
    i2cBuffer[1] =  (adc_buffer>>8) & 0xFF;
    i2cBuffer[2] =  adc_buffer & 0xFF; 
}

// I2C command to read timed accumulation of analog channel ALT_I
void fnRdTimedAccumAltI(uint8_t* i2cBuffer)
{
    // there are four bytes in the unsigned long accumulate_alt_ti
    i2cBuffer[1] =  (accumulate_alt_ti>>24) & 0xFF; // high byte. Mask is for clarity, the compiler should optimize it out
    i2cBuffer[2] =  (accumulate_alt_ti>>16) & 0xFF;
    i2cBuffer[3] =  (accumulate_alt_ti>>8) & 0xFF;
    i2cBuffer[4] =  accumulate_alt_ti & 0xFF; // low byte. Again Mask should optimize out
}

// I2C command to read timed accumulation of analog channel PWR_I
void fnRdTimedAccumPwrI(uint8_t* i2cBuffer)
{
    // there are four bytes in the unsigned long accumulate_alt_ti
    i2cBuffer[1] =  (accumulate_pwr_ti>>24) & 0xFF;
    i2cBuffer[2] =  (accumulate_pwr_ti>>16) & 0xFF;
    i2cBuffer[3] =  (accumulate_pwr_ti>>8) & 0xFF;
    i2cBuffer[4] =  accumulate_pwr_ti & 0xFF;
}

// I2C command for Analog referance EXTERNAL_AVCC
// swap the I2C buffer with the ref_extern_avcc_uV in use
// set ref_loaded so main loop will try to save it to EEPROM
// the main loop will reload EEPROM or default value if new is out of range
void fnAnalogRefExternAVCC(uint8_t* i2cBuffer)
{
    // I work with ref_extern_avcc_uV as a uint32_t, but it is a float (both are four bytes)
    uint32_t old = ref_extern_avcc_uV;
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
    ref_extern_avcc_uV = new;

    ref_loaded = REF_AVCC_TOSAVE; // main loop will save to eeprom or load default value if new value is out of range
}

// I2C command for Analog referance INTERNAL_1V1
void fnAnalogRefIntern1V1(uint8_t* i2cBuffer)
{
    // I work with ref_extern_avcc_uV as a uint32_t, but it is a float (both are four bytes)
    uint32_t old = ref_intern_1v1_uV;
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
    ref_intern_1v1_uV = new;
    
    ref_loaded = REF_1V1_TOSAVE; // main loop will save to eeprom or load default value if new value is out of range
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
            uart_started_at = millis();
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
            digitalWrite(DTR_TXD,HIGH); // strong pullup
            pinMode(DTR_TXD,INPUT); // the DTR pair driver will see a weak pullup when UART starts
            UCSR0B |= (1<<RXEN0)|(1<<TXEN0); // turn on UART
            digitalWrite(DTR_DE, HIGH); //DTR transceiver may have been turned off during the test
            digitalWrite(DTR_nRE, LOW); 
            uart_started_at = millis();
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
        i2cBuffer[1] = ( (digitalRead(HOST_nRTS)<<7) | (digitalRead(HOST_nCTS)<<6) | (digitalRead(TX_nRE)<<5) | (digitalRead(TX_DE)<<4) | (digitalRead(DTR_nRE)<<3) | (digitalRead(DTR_DE)<<2) | (digitalRead(RX_nRE)<<1) | (digitalRead(RX_DE)) ); 
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
        // mask the needed bit and shift it to position zero so digitalWrite can move it to where it needs to go.
        digitalWrite(HOST_nRTS, ( (i2cBuffer[1] & (1<<7))>>7 ) );
        digitalWrite(HOST_nCTS, ( (i2cBuffer[1] & (1<<6))>>6 ) );
        digitalWrite(TX_nRE, ( (i2cBuffer[1] & (1<<5))>>5 ) );
        digitalWrite(TX_DE, ( (i2cBuffer[1] & (1<<4))>>4 ) );
        digitalWrite(DTR_nRE, ( (i2cBuffer[1] & (1<<3))>>3 ) ); // setting this will blind others state change but I need it for testing
        if ( (i2cBuffer[1] & (1<<2))>>2 ) // enabling the dtr driver in testmode needs to cause a transcever load on the dtr pair
        {
            UCSR0B &= ~( (1<<RXEN0)|(1<<TXEN0) ); // turn off UART 
            pinMode(DTR_TXD,OUTPUT);
            digitalWrite(DTR_TXD,LOW); // the DTR pair will be driven and load the transceiver 
            digitalWrite(DTR_DE,  1); 
        }
        digitalWrite(RX_nRE, ( (i2cBuffer[1] & (1<<1))>>1 ) );
        digitalWrite(RX_DE,  (i2cBuffer[1] & 1) );
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

    daynight_values_loaded = DAYNIGHT_MORNING_DEBOUNCE_TOSAVE; // main loop will save to eeprom or load default value if new value is out of range
}

// I2C command to read millis time
void fnMillis(uint8_t* i2cBuffer)
{
    unsigned long now = millis();
    // there are four bytes in an unsigned long
    i2cBuffer[1] = ( (0xFF000000UL & now) >>24 ); 
    i2cBuffer[2] =  ( (0x00FF0000UL & now) >>16 ); 
    i2cBuffer[3] =  ( (0x0000FF00UL & now) >>8 ); 
    i2cBuffer[4] =  ( (0x000000FFUL & now) );
}

/* Dummy function */
void fnNull(uint8_t* i2cBuffer)
{
    return; 
}