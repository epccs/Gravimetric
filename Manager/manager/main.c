/* manager is a RPUBUS firmware
(c) 2020 Ronald Sutherland.

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
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "../lib/timers_bsd.h"
#include "../lib/twi0_bsd.h"
#include "../lib/twi1_bsd.h"
#include "../lib/uart0_bsd.h"
#include "../lib/io_enum_bsd.h"
#include "main.h"
#include "rpubus_manager_state.h"
#include "dtr_transmition.h"
#include "i2c_cmds.h"
#include "smbus_cmds.h"
#include "id_in_ee.h"
#include "adc_burst.h"
#include "references.h"
#include "battery_manager.h"
#include "battery_limits.h"
#include "daynight_limits.h"
#include "daynight_state.h"
#include "host_shutdown_limits.h"
#include "host_shutdown_manager.h"
#include "calibration_limits.h"

void setup(void) 
{
    ioWrite(MCU_IO_PIPWR_EN, LOGIC_LEVEL_LOW); // power down the SBC
    ioDir(MCU_IO_PIPWR_EN, DIRECTION_OUTPUT);
    ioDir(MCU_IO_MGR_SCK_LED, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_MGR_SCK_LED, LOGIC_LEVEL_HIGH);
    ioDir(MCU_IO_HOST_nRTS, DIRECTION_INPUT);
    ioWrite(MCU_IO_HOST_nRTS, LOGIC_LEVEL_HIGH); // with AVR when the pin DDR is set as an input setting pin high will trun on a weak pullup 
    ioDir(MCU_IO_HOST_nCTS, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_HOST_nCTS, LOGIC_LEVEL_HIGH);
    ioDir(MCU_IO_RX_DE, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_RX_DE, LOGIC_LEVEL_HIGH);  // allow RX pair driver to enable if FTDI_TX is low
    ioDir(MCU_IO_RX_nRE, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_RX_nRE, LOGIC_LEVEL_LOW);  // enable RX pair recevior to output to local MCU's RX input
    ioDir(MCU_IO_TX_DE, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_TX_DE, LOGIC_LEVEL_HIGH); // allow TX pair driver to enable if TX (from MCU) is low
    ioDir(MCU_IO_TX_nRE, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_TX_nRE, LOGIC_LEVEL_LOW);  // enable TX pair recevior to output to FTDI_RX input
    ioDir(MCU_IO_DTR_DE, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_DTR_DE, LOGIC_LEVEL_LOW);  // use this to block UART startup glitch
    ioDir(MCU_IO_DTR_nRE, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_DTR_nRE, LOGIC_LEVEL_LOW); 
    ioDir(MCU_IO_MGR_nSS, DIRECTION_OUTPUT); // nSS is input to a Open collector buffer used to pull to MCU nRESET low
    ioWrite(MCU_IO_MGR_nSS, LOGIC_LEVEL_HIGH); 
    ioDir(MCU_IO_ALT_EN, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_ALT_EN, LOGIC_LEVEL_LOW);

    // from rpubus_manager_state.h
    bootloader_address = RPU_HOST_CONNECT; 
    host_active = 0;
    lockout_active = 0;
    status_byt = 0;
    write_rpu_address_to_eeprom = 0;
    shutdown_detected = 0;
    shutdown_started = 0;
    arduino_mode_started =0;
    arduino_mode = 0;
    test_mode_started = 0;
    test_mode = 0;
    transceiver_state = 0;
    
    // from smbus_cmds.h
    smbus_has_numBytes_to_handle = 0;
    
    // from dtr_transmition.h
    uart_output = 0;

    //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.
    initTimers();

    // Initialize ADC and put in Auto Trigger mode to fetch an array of channels
    init_ADC_single_conversion(EXTERNAL_AVCC); // warning AREF must not be connected to anything
    enable_ADC_auto_conversion(BURST_MODE);
    adc_started_at = milliseconds();

    /* Initialize UART0 to 250 kbps, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stdout = stdin = uart0_init(DTR_BAUD,UART0_RX_REPLACE_CR_WITH_NL);

    // can use with a true I2C bus master that does clock stretching and repeated starts 
    twi0_slaveAddress(I2C0_ADDRESS);
    twi0_registerSlaveTxCallback(transmit_i2c_event); // called when I2C slave has been requested to send data
    twi0_registerSlaveRxCallback(receive_i2c_event); // called when I2C slave has received data
    twi0_init(100000UL, TWI0_PINS_FLOATING); // do not use internal pull-up

    // with interleaved buffer for use with SMbus bus master that does not like clock-stretching (e.g., R-Pi Zero) 
    twi1_slaveAddress(I2C1_ADDRESS);
    twi1_registerSlaveTxCallback(transmit_smbus_event); // called when SMBus slave has been requested to send data
    twi1_registerSlaveRxCallback(receive_smbus_event); // called when SMBus slave has received data
    twi1_init(100000UL, TWI1_PINS_FLOATING); // do not use internal pull-up a Raspberry Pi has them on board

    sei(); // Enable global interrupts to start TIMER0 and UART

    _delay_ms(50); // wait for UART glitch to clear, blocking at this point is OK.
    ioWrite(MCU_IO_DTR_DE, LOGIC_LEVEL_HIGH);  // then allow DTR pair driver to enable

    // load references
    ref_loaded = REF_CLEAR;
    for (uint8_t ref_index = 0; ref_index < REFERENCE_OPTIONS; ref_index++)
    {
        LoadRefFromEEPROM((REFERENCE_t) ref_index);
    }

    // Use eeprom value for rpu_address if ID was valid    
    if (check_for_eeprom_id())
    {
        rpu_address = eeprom_read_byte((uint8_t*)(EE_RPU_ADDRESS));
    }
    else
    {
        rpu_address = RPU_ADDRESS;
    }

    // load Battery Limits from EEPROM (or set defaults)
    LoadBatLimitsFromEEPROM();

    // load host shutdown parameters from EEPROM (or set defaults)
    LoadShtDwnLimitsFromEEPROM();

    // load Day-Night state machine values from EEPROM (or set defaults)
    LoadDayNightValuesFromEEPROM();
    daynight_timer = milliseconds();

    // load Calibrations for for adc channels ALT_I, ALT_V,PWR_I,PWR_V
    cal_loaded = CAL_CLEAR;
    for (uint8_t cal_index = 0; cal_index < ADC_ENUM_END; cal_index++)
    {
        LoadCalFromEEPROM( (ADC_ENUM_t) cal_index);
    }

    // The manager's default is to power up the host when power is applied even if it is held in reset.
    // The goal now is to lockout the shutdown switch for some amount of time to match what the hs daemon does.
    ioDir(MCU_IO_PIPWR_EN, DIRECTION_INPUT);
    ioWrite(MCU_IO_PIPWR_EN, LOGIC_LEVEL_HIGH); // power up the SBC (a pull up does this when in reset e.g., hi-z)
    ioDir(MCU_IO_SHUTDOWN, DIRECTION_OUTPUT); 
    ioWrite(MCU_IO_SHUTDOWN, LOGIC_LEVEL_HIGH); // lockout manual shutdown switch
    shutdown_state = HOSTSHUTDOWN_STATE_RESTART_DLY;

#if defined(DISCONNECT_AT_PWRUP)
    // at power up send a byte on the DTR pair to unlock the bus 
    // problem is if a foreign host has the bus this would be bad
    uart_started_at = milliseconds();
    uart_output= RPU_HOST_DISCONNECT;
    printf("%c", uart_output); 
#endif
#if defined(HOST_LOCKOUT)
    // this will keep the host off the bus until the HOST_LOCKOUT_STATUS bit in status_byt is clear 
    // status_byt was zero at this point, but this sets the bit without changing the other bits
    status_byt |= (1<<HOST_LOCKOUT_STATUS);
#endif
}

int main(void)
{
    setup();

    blink_started_at = milliseconds();

    while (1) // scan time for each loop varies depending on how much of each thing needs to be done 
    {
        if (!test_mode) 
        {
            blink_on_activate();
            check_Bootload_Time();
            check_DTR();
            check_lockout();
        }
        save_rpu_addr_state();
        check_uart();
        adc_burst();
        ReferanceFromI2CtoEE();
        ChannelCalFromI2CtoEE();
        BatLimitsFromI2CtoEE();
        check_battery_manager();
        DayNightValuesFromI2CtoEE();
        check_daynight();
        ShtDwnLimitsFromI2CtoEE();
        check_if_host_should_be_on();
        handle_smbus_receive();
    }    
}

