/* Remote is a RPUBUS manager firmware
Copyright (C) 2019 Ronald Sutherland

 This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

For a copy of the GNU General Public License use
http://www.gnu.org/licenses/gpl-2.0.html
 */ 

#include <util/delay.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "../lib/timers.h"
#include "../lib/twi0.h"
#include "../lib/twi1.h"
#include "../lib/uart.h"
#include "../lib/adc.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"
#include "main.h"
#include "rpubus_manager_state.h"
#include "dtr_transmition.h"
#include "i2c_cmds.h"
#include "smbus_cmds.h"
#include "id_in_ee.h"
#include "adc_burst.h"
#include "references.h"
#include "power_manager.h"
#include "battery_limits.h"
#include "daynight_limits.h"
#include "daynight_state.h"

void setup(void) 
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    pinMode(HOST_nRTS, INPUT);
    digitalWrite(HOST_nRTS, HIGH); // with AVR when the pin DDR is set as an input setting pin high will trun on a weak pullup 
    pinMode(HOST_nCTS, OUTPUT);
    digitalWrite(HOST_nCTS, HIGH);
    pinMode(RX_DE, OUTPUT);
    digitalWrite(RX_DE, HIGH);  // allow RX pair driver to enable if FTDI_TX is low
    pinMode(RX_nRE, OUTPUT);
    digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
    pinMode(TX_DE, OUTPUT);
    digitalWrite(TX_DE, HIGH); // allow TX pair driver to enable if TX (from MCU) is low
    pinMode(TX_nRE, OUTPUT);
    digitalWrite(TX_nRE, LOW);  // enable TX pair recevior to output to FTDI_RX input
    pinMode(DTR_DE, OUTPUT);
    digitalWrite(DTR_DE, LOW);  // use this to block UART startup glitch
    pinMode(DTR_nRE, OUTPUT);
    digitalWrite(DTR_nRE, LOW); 
    pinMode(MGR_nSS, OUTPUT); // nSS is input to a Open collector buffer used to pull to MCU nRESET low
    digitalWrite(MGR_nSS, HIGH); 
    pinMode(SHUTDOWN, INPUT);
    digitalWrite(SHUTDOWN, HIGH); // trun on a weak pullup 

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
    adc_started_at = millis();

    /* Initialize UART, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stdout = stdin = uartstream0_init(DTR_BAUD);

    // can use with a true I2C bus master that does clock stretching and repeated starts 
    twi0_setAddress(I2C0_ADDRESS);
    twi0_attachSlaveTxEvent(transmit_i2c_event); // called when I2C slave has been requested to send data
    twi0_attachSlaveRxEvent(receive_i2c_event); // called when I2C slave has received data
    twi0_init(false); // do not use internal pull-up

    // with interleaved buffer for use with SMbus bus master that does not like clock-stretching (e.g., R-Pi Zero) 
    twi1_setAddress(I2C1_ADDRESS);
    twi1_attachSlaveTxEvent(transmit_smbus_event); // called when SMBus slave has been requested to send data
    twi1_attachSlaveRxEvent(receive_smbus_event); // called when SMBus slave has received data
    twi1_init(false); // do not use internal pull-up a Raspberry Pi has them on board

    sei(); // Enable global interrupts to start TIMER0 and UART

    _delay_ms(50); // wait for UART glitch to clear, blocking at this point is OK.
    digitalWrite(DTR_DE, HIGH);  // then allow DTR pair driver to enable

    // load reference calibration
    LoadAnalogRefFromEEPROM();

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

    // load Day-Night state machine values from EEPROM (or set defaults)
    LoadDayNightValuesFromEEPROM();
    dayTmrStarted = millis();

#if defined(DISCONNECT_AT_PWRUP)
    // at power up send a byte on the DTR pair to unlock the bus 
    // problem is if a foreign host has the bus this would be bad
    uart_started_at = millis();
    uart_output= RPU_HOST_DISCONNECT;
    printf("%c", uart_output); 
#endif
#if defined(HOST_LOCKOUT)
// this will keep the host off the bus until the HOST_LOCKOUT_STATUS bit in status_byt is clear 
// status_byt is zero at this point, but this shows how to set the bit without changing other bits
    status_byt |= (1<<HOST_LOCKOUT_STATUS);
#endif
}

int main(void)
{
    setup();

    blink_started_at = millis();

    while (1) // scan time for each loop varies depending on how much of each thing needs to be done 
    {
        if (!test_mode) 
        {
            blink_on_activate();
            check_Bootload_Time();
            check_DTR();
            check_lockout();
            check_shutdown();
        }
        save_rpu_addr_state();
        check_uart();
        adc_burst();
        CalReferancesFromI2CtoEE();
        BatLimitsFromI2CtoEE();
        check_if_alt_should_be_on();
        DayNightValuesFromI2CtoEE();
        check_daynight();
        handle_smbus_receive();
    }    
}

