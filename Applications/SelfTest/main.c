/* 
Gravimetric SelfTest
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

#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/io.h>
#include "../lib/timers.h"
#include "../lib/uart.h"
#include "../lib/twi0.h"
#include "../lib/twi1.h"
#include "../lib/adc.h"
#include "../lib/rpu_mgr.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"
#include "../Adc/references.h"

#define BLINK_DELAY 1000UL

// Save the Value of the References for ADC converter 
// measure AVCC and put it hear in uV 
#define REF_EXTERN_AVCC 4958300UL


#define ICP3AND4_TERM 50.0
#define ICP1_TERM 100.0

static unsigned long blink_started_at;
static unsigned long blink_delay;
static char rpu_addr;
static uint8_t passing;
static uint8_t test_mode_clean;
static uint8_t delayed_outputs;
static uint8_t delayed_data[8];

void setup_pins_off(void)
{
    // Turn Off Curr Sources
    pinMode(CS0_EN,OUTPUT);
    digitalWrite(CS0_EN,LOW);
    pinMode(CS1_EN,OUTPUT);
    digitalWrite(CS1_EN,LOW);
    pinMode(CS2_EN,OUTPUT);
    digitalWrite(CS2_EN,LOW);
    pinMode(CS3_EN,OUTPUT);
    digitalWrite(CS3_EN,LOW);
    pinMode(CS4_EN,OUTPUT);
    digitalWrite(CS4_EN,LOW);
    pinMode(CS_ICP1,OUTPUT);
    digitalWrite(CS_ICP1,LOW);
    pinMode(CS_ICP3,OUTPUT);
    digitalWrite(CS_ICP3,LOW);
    pinMode(CS_ICP4,OUTPUT);
    digitalWrite(CS_ICP4,LOW);
    pinMode(CS_FAST,OUTPUT);
    digitalWrite(CS_FAST,LOW);
    pinMode(CS_DIVERSION,OUTPUT);
    digitalWrite(CS_DIVERSION,LOW);

    // DIO and Analog
    pinMode(RX1,INPUT);
    digitalWrite(RX1,LOW);
    pinMode(TX1,INPUT);
    digitalWrite(TX1,LOW);
    pinMode(RX2,INPUT);
    digitalWrite(RX2,LOW);
    pinMode(TX2,INPUT);
    digitalWrite(TX2,LOW);
    pinMode(SDA1,INPUT);
    digitalWrite(SDA1,LOW);
    pinMode(SCL1,INPUT);
    digitalWrite(SCL1,LOW);
    pinMode(28,INPUT); // ADC0
    pinMode(29,INPUT);
    pinMode(30,INPUT);
    pinMode(31,INPUT);
    pinMode(32,INPUT);
    pinMode(33,INPUT);
    pinMode(34,INPUT);
    pinMode(35,INPUT); // ADC7
    
    // R-Pi power control
    pinMode(SHLD_VIN_EN,OUTPUT);
    digitalWrite(SHLD_VIN_EN,LOW);
    
    // Alternate power control
    pinMode(ALT_EN,OUTPUT);
    digitalWrite(ALT_EN,LOW);
    
    // SPI needs a loopback on the R-Pi connector between PI_MISO and PI_MOSI
    pinMode(MISO,INPUT);
    digitalWrite(MISO,HIGH); //a weak pull up will turn off the buffer that would otherwise pull down ICP3/MOSI
}


void setup(void) 
{
    setup_pins_off();

    // Initialize Timers, ADC, and clear bootloader, Arduino does these with init() in wiring.c
    initTimers(); //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.
    init_ADC_single_conversion(EXTERNAL_AVCC); // warning AREF must not be connected to anything
    init_uart0_after_bootloader(); // bootloader may have the UART setup

    /* Initialize UART, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stdout = stdin = uartstream0_init(BAUD);

    /* Initialize I2C, with the internal pull-up*/
    twi0_init(TWI_PULLUP);
    twi1_init(TWI_PULLUP);

    // Enable global interrupts to start TIMER0 and UART
    sei(); 

    blink_started_at = millis();

    rpu_addr = get_Rpu_address();
    blink_delay = BLINK_DELAY;

    // blink fast if a default address from RPU manager not found
    if (rpu_addr == 0)
    {
        rpu_addr = '0';
        blink_delay = BLINK_DELAY/4;
    }
    
    // set the referances and save them in EEPROM
    ref_extern_avcc_uV = REF_EXTERN_AVCC;
}

void smbus_address(void)
{
    uint8_t smbus_address = 0x2A;
    uint8_t length = 2;
    uint8_t wait = 1;
    uint8_t sendStop = 1;
    uint8_t txBuffer[2] = {0x00,0x00}; //comand 0x00 should Read the mulit-drop bus addr;
    uint8_t twi1_returnCode = twi1_writeTo(smbus_address, txBuffer, length, wait, sendStop); 
    if (twi1_returnCode != 0)
    {
        passing = 0; 
        printf_P(PSTR(">>> SMBus write failed, twi1_returnCode: %d\r\n"), twi1_returnCode);
    }
    
    // read_i2c_block_data sends a command byte and then a repeated start followed by reading the data 
    uint8_t cmd_length = 1; // one byte command is sent befor read with the read_i2c_block_data
    sendStop = 0; // a repeated start happens after the command byte is sent
    txBuffer[0] = 0x00; //comand 0x00 matches the above write command
    twi1_returnCode = twi1_writeTo(smbus_address, txBuffer, cmd_length, wait, sendStop); 
    if (twi1_returnCode != 0)
    {
        passing = 0; 
        printf_P(PSTR(">>> SMBus read cmd fail, twi1_returnCode: %d\r\n"), twi1_returnCode);
    }
    uint8_t rxBuffer[2] = {0x00,0x00};
    sendStop = 1;
    uint8_t bytes_read = twi1_readFrom(smbus_address, rxBuffer, length, sendStop);
    if ( bytes_read != length )
    {
        passing = 0; 
        printf_P(PSTR(">>> SMBus read missing %d bytes \r\n"), (length-bytes_read) );
    }
    if ( (rxBuffer[0] == 0x0) && (rxBuffer[1] == '1') )
    {
        printf_P(PSTR("SMBUS cmd %d provided address %d from manager\r\n"), rxBuffer[0], rxBuffer[1]);
    } 
    else  
    { 
        passing = 0; 
        printf_P(PSTR(">>> SMBUS wrong addr %d for cmd %d\r\n"), rxBuffer[1], rxBuffer[0]);
    }
}

void i2c_shutdown(void)
{
    uint8_t i2c_address = 0x29;
    uint8_t length = 2;
    uint8_t wait = 1;
    uint8_t sendStop = 0; // use a repeated start after write
    uint8_t txBuffer[2] = {0x05,0x01};
    uint8_t twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop); 
    if (twi_returnCode != 0)
    {
        passing = 0; 
        printf_P(PSTR(">>> I2C0 cmd 5 write fail, twi_returnCode: %d\r\n"), twi_returnCode);
        return;
    }
    uint8_t rxBuffer[2] = {0x00,0x00};
    sendStop = 1;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, length, sendStop);
    if ( bytes_read != length )
    {
        passing = 0; 
        printf_P(PSTR(">>> I2C0 read missing %d bytes \r\n"), (length-bytes_read) );
    }
    if ( (txBuffer[0] == rxBuffer[0]) && (txBuffer[1] == rxBuffer[1]) )
    {
        printf_P(PSTR("I2C0 Shutdown cmd is clean {%d, %d}\r\n"), rxBuffer[0], rxBuffer[1]);
    } 
    else  
    { 
        passing = 0; 
        printf_P(PSTR(">>> I2C0 Shutdown cmd echo bad {%d, %d}\r\n"), rxBuffer[0], rxBuffer[1]);
    }
}

void i2c_shutdown_detect(void)
{
    uint8_t i2c_address = 0x29;
    uint8_t length = 2;
    uint8_t wait = 1;
    uint8_t sendStop = 0; // use a repeated start after write
    uint8_t txBuffer[2] = {0x04, 0xFF}; //comand 0x04 will return the value 0x01 (0xff is a byte for the ISR to replace)
    uint8_t twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop);
    if (twi_returnCode != 0)
    {
        passing = 0; 
        printf_P(PSTR(">>> I2C0 cmd 4 write fail, twi_returnCode: %d\r\n"), twi_returnCode);
    }
    uint8_t rxBuffer[2] = {0x00,0x00};
    sendStop = 1;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, length, sendStop);
    if ( bytes_read != length )
    {
        passing = 0; 
        printf_P(PSTR(">>> I2C0 read missing %d bytes \r\n"), (length-bytes_read) );
    }
    if ( (txBuffer[0] == rxBuffer[0]) && (0x1 == rxBuffer[1]) )
    {
        printf_P(PSTR("I2C0 Shutdown Detect cmd is clean {%d, %d}\r\n"), rxBuffer[0], rxBuffer[1]);
    } 
    else  
    { 
        passing = 0; 
        printf_P(PSTR(">>> I2C0 Shutdown Detect cmd echo bad {%d, %d}\r\n"), rxBuffer[0], rxBuffer[1]);
    }
}

void i2c_testmode_start(void)
{
    uint8_t i2c_address = 0x29;
    uint8_t length = 2;
    uint8_t wait = 1;
    uint8_t sendStop = 0; // use a repeated start after write
    uint8_t txBuffer[2] = {0x30, 0x01}; // preserve the trancever control bits
    uint8_t twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop);
    if (twi_returnCode != 0)
    {
        passing = 0; 
        printf_P(PSTR(">>> I2C0 cmd 48 write fail, twi_returnCode: %d\r\n"), twi_returnCode);
    }
    uint8_t rxBuffer[2] = {0x00,0x00};
    sendStop = 1;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, length, sendStop);
    if ( bytes_read != length )
    {
        passing = 0; 
        printf_P(PSTR(">>> I2C0 read missing %d bytes \r\n"), (length-bytes_read) );
    }
    if ( (txBuffer[0] == rxBuffer[0]) && (txBuffer[1]  == rxBuffer[1]) )
    {
        test_mode_clean = 1;
        // delay print until the UART can be used
    } 
    else  
    { 
        passing = 0; 
        printf_P(PSTR(">>> I2C0 Start Test Mode cmd echo bad {%d, %d}\r\n"), rxBuffer[0], rxBuffer[1]);
    }
}

void i2c_testmode_end(void)
{
    uint8_t i2c_address = 0x29;
    uint8_t length = 2;
    uint8_t wait = 1;
    uint8_t sendStop = 0; // use a repeated start after write
    uint8_t txBuffer[2] = {0x31, 0x01}; // recover trancever control bits and report values in data[1] byte
    uint8_t twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop);
    if ( test_mode_clean )
    {
        printf_P(PSTR("I2C0 Start Test Mode cmd was clean {48, 1}\r\n"));
        test_mode_clean = 0;
    }
    if (twi_returnCode != 0)
    {
        passing = 0; 
        printf_P(PSTR(">>> I2C0 cmd 49 write fail, twi_returnCode: %d\r\n"), twi_returnCode);
    }
    uint8_t rxBuffer[2] = {0x00,0x00};
    sendStop = 1;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, length, sendStop);
    if ( bytes_read != length )
    {
        passing = 0; 
        printf_P(PSTR(">>> I2C0 read missing %d bytes \r\n"), (length-bytes_read) );
    }
    if ( (txBuffer[0] == rxBuffer[0]) && (0xD5 == rxBuffer[1]) )
    {
        printf_P(PSTR("I2C0 End Test Mode hex is Xcvr cntl bits {%d, 0x%X}\r\n"), rxBuffer[0], rxBuffer[1]);
    } 
    else  
    { 
        passing = 0; 
        printf_P(PSTR(">>> I2C0 problem /w End Test Mode {%d, %d}\r\n"), rxBuffer[0], rxBuffer[1]);
    }
}

void i2c_testmode_test_xcvrbits(uint8_t xcvrbits)
{
    // do not use UART durring testmode
    if ( test_mode_clean )
    {
        uint8_t i2c_address = 0x29;
        uint8_t length = 2;
        uint8_t wait = 1;
        uint8_t sendStop = 0; // use a repeated start after write
        uint8_t txBuffer[2] = {0x32, 0x01};

        uint8_t twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop);
        if (twi_returnCode != 0)
        {
            passing = 0;
            delayed_outputs |= (1<<4); // >>> I2C cmd 50 write fail, twi_returnCode: %d\r\n
            delayed_data[4] =  twi_returnCode;
        }
        uint8_t rxBuffer[2] = {0x00,0x00};
        sendStop = 1;
        uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, length, sendStop);
        if ( bytes_read != length )
        {
            passing = 0; 
            delayed_outputs |= (1<<5); // >>> I2C read missing %d bytes \r\n
            delayed_data[5] =  length-bytes_read;
        }
        if ( (txBuffer[0] == rxBuffer[0]) && (xcvrbits == rxBuffer[1]) )
        { //0xe2 is 0b11100010
            // HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE
            delayed_outputs |= (1<<6); // Testmode: read  Xcvr cntl bits {50, 0x%X}\r\n
            delayed_data[6] =  rxBuffer[1];
        } 
        else  
        { 
            passing = 0;
            delayed_outputs |= (1<<7); // >>> Xcvr cntl bits should be %x but report was %x
            delayed_data[7] =  rxBuffer[1]; // used with second format output
        }
    }
}

void i2c_testmode_set_xcvrbits(uint8_t xcvrbits)
{
    // do not use UART durring testmode
    if ( test_mode_clean )
    {
        uint8_t i2c_address = 0x29;
        uint8_t length = 2;
        uint8_t wait = 1;
        uint8_t sendStop = 0; // use a repeated start after write
        uint8_t txBuffer[2];
        txBuffer[0] = 0x33;
        txBuffer[1] = xcvrbits;
        uint8_t twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop);
        if (twi_returnCode != 0)
        {
            passing = 0;
            delayed_outputs |= (1<<0); // >>> I2C cmd 51 write fail, twi_returnCode: %d\r\n
            delayed_data[0] =  twi_returnCode;
        }
        uint8_t rxBuffer[2] = {0x00,0x00};
        sendStop = 1;
        uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, length, sendStop);
        if ( bytes_read != length )
        {
            passing = 0; 
            delayed_outputs |= (1<<1); // >>> I2C read missing %d bytes \r\n
            delayed_data[1] =  length-bytes_read;
        }
        if ( (txBuffer[0] == rxBuffer[0]) && (xcvrbits == rxBuffer[1]) )
        { //0xe2 is 0b11100010
            // HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE
            delayed_outputs |= (1<<2); // Testmode: set  Xcvr cntl bits {51, 0x%X}\r\n
            delayed_data[2] =  rxBuffer[1];
        } 
        else  
        { 
            passing = 0;
            delayed_outputs |= (1<<3); // >>> Xcvr cntl bits set as %x but report was %x
            delayed_data[3] =  rxBuffer[1]; // used with second format output
        }
    }
}

void test(void)
{
    // Info from some Predefined Macros
    printf_P(PSTR("Gravimetric Self Test date: %s\r\n"), __DATE__);
    printf_P(PSTR("avr-gcc --version: %s\r\n"),__VERSION__);
    
    // I2C is used to read serial bus manager address 
    if (rpu_addr == '1')
    {
        printf_P(PSTR("I2C provided address 0x31 from RPUadpt serial bus manager\r\n"));
    } 
    else  
    { 
        passing = 0; 
        printf_P(PSTR(">>> I2C failed, or address not 0x31 from serial bus manager\r\n"));
        return;
    }

    // With current sources off measure input current
    _delay_ms(1000) ; // busy-wait to let the 1uF settle
    
    // Input voltage
    int adc_pwr_v = analogRead(PWR_V);
    printf_P(PSTR("adc reading for PWR_V: %d int\r\n"), adc_pwr_v);
    float input_v = adc_pwr_v*((ref_extern_avcc_uV/1.0E6)/1024.0)*(115.8/15.8);
    printf_P(PSTR("PWR at: %1.3f V\r\n"), input_v);
    if (input_v > 14.0) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> Input voltage is to high.\r\n"));
        return;
    }
    if (input_v < 12.0) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> Input voltage is to low.\r\n"));
        return;
    }

    //current sources are off, measure ADC0..ADC3 
    float adc0_v = analogRead(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    printf_P(PSTR("ADC0 at ICP3&4 TERM /W all CS off: %1.3f V\r\n"), adc0_v);
    float adc1_v = analogRead(ADC1)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    printf_P(PSTR("ADC1 at ICP1 TERM /w all CS off: %1.3f V\r\n"), adc1_v);
    float adc2_v = analogRead(ADC2)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    printf_P(PSTR("ADC2 at ICP3&4 TERM /W all CS off: %1.3f V\r\n"), adc2_v);
    float adc3_v = analogRead(ADC3)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    printf_P(PSTR("ADC3 at ICP3&4 TERM /W all CS off: %1.3f V\r\n"), adc3_v);
    if ( (adc0_v > 0.01)  || (adc1_v > 0.01) || (adc2_v > 0.01) || (adc3_v > 0.01))
    { 
        passing = 0; 
        printf_P(PSTR(">>> ADC is to high, is the self-test wiring right?\r\n"));
        return;
    }

    // ICP1 pin is inverted from the plug interface, its 100 Ohm Termination should have zero mA. 
    printf_P(PSTR("ICP1 input should be HIGH with 0mA loop current: %d \r\n"), digitalRead(ICP1));
    if (!digitalRead(ICP1)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP1 should be high.\r\n"));
    }

    // enable CS_ICP1 (with green LED)
    digitalWrite(CS_ICP1,HIGH);
    _delay_ms(100); // busy-wait delay

    // ICP1_TERM has CS_ICP1 on it
    float adc1_cs_icp1_v = analogRead(ADC1)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc1_cs_icp1_i = adc1_cs_icp1_v / ICP1_TERM;
    printf_P(PSTR("CS_ICP1 on ICP1 TERM: %1.3f A\r\n"), adc1_cs_icp1_i);
    if (adc1_cs_icp1_i < 0.012) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_ICP1 curr is to low.\r\n"));
    }
    if (adc1_cs_icp1_i > 0.020) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_ICP1 curr is to high.\r\n"));
    }

    // ICP1 pin is inverted from to the plug interface, which should have 17 mA on its 100 Ohm Termination now
    printf_P(PSTR("ICP1 /w 17mA on termination reads: %d \r\n"), digitalRead(ICP1));
    if (digitalRead(ICP1)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP1 should be low with 17mA.\r\n"));
    }
    digitalWrite(CS_ICP1,LOW);

    // ICP3 pin is inverted from the plug interface, its Termination should have zero mA. 
    printf_P(PSTR("ICP3 input should be HIGH with 0mA loop current: %d \r\n"), digitalRead(ICP3));
    if (!digitalRead(ICP3)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP3 should be high.\r\n"));
    }

    // enable CS_ICP3
    digitalWrite(CS_ICP3,HIGH);
    _delay_ms(100); // busy-wait delay

    // ICP3_TERM and ICP4_TERM (50Ohm) has CS_ICP3 on it
    float adc0_cs_icp3_v = analogRead(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc0_cs_icp3_i = adc0_cs_icp3_v / ICP3AND4_TERM;
    printf_P(PSTR("CS_ICP3 on ICP3AND4 TERM: %1.3f A\r\n"), adc0_cs_icp3_i);
    if (adc0_cs_icp3_i < 0.012) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_ICP3 curr is to low.\r\n"));
    }
    if (adc0_cs_icp3_i > 0.020) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_ICP3 curr is to high.\r\n"));
    }

    // ICP3 pin is inverted from to the plug interface, which should have 17 mA on its 100 Ohm Termination now
    printf_P(PSTR("ICP3 /w 8mA on termination reads: %d \r\n"), digitalRead(ICP3));
    if (digitalRead(ICP1)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP3 should be low with 8mA.\r\n"));
    }

    //Swap ADC referances and find the band-gap voltage
    init_ADC_single_conversion(INTERNAL_1V1); 
    _delay_ms(100); // busy-wait delay
    int adc0_used_for_ref_intern_1v1_uV = analogRead(ADC0);
    printf_P(PSTR("   ADC0 reading used to calculate ref_intern_1v1_uV: %d int\r\n"), adc0_used_for_ref_intern_1v1_uV);
    float _ref_intern_1v1_uV = 1.0E6*1024.0 * ((adc0_cs_icp3_i * ICP3AND4_TERM) / adc0_used_for_ref_intern_1v1_uV);
    uint32_t temp_ref_intern_1v1_uV = (uint32_t)_ref_intern_1v1_uV;
    printf_P(PSTR("   calculated ref_intern_1v1_uV: %lu uV\r\n"), temp_ref_intern_1v1_uV);
    uint32_t temp_ref_extern_avcc_uV = ref_extern_avcc_uV;
    
    // check for old referance values
    ref_extern_avcc_uV = 0;
    ref_intern_1v1_uV = 0;
    LoadAnalogRefFromEEPROM();
    printf_P(PSTR("REF_EXTERN_AVCC old value found in eeprom: %lu uV\r\n"), ref_extern_avcc_uV);
    printf_P(PSTR("REF_INTERN_1V1 old value found in eeprom: %lu uV\r\n"), ref_intern_1v1_uV);
    ref_intern_1v1_uV = temp_ref_intern_1v1_uV;
    if (ref_extern_avcc_uV == temp_ref_extern_avcc_uV)
    {
        printf_P(PSTR("REF_EXTERN_AVCC from eeprom is same\r\n"));
    }
    else
    {
        ref_extern_avcc_uV = temp_ref_extern_avcc_uV;
        if ((ref_intern_1v1_uV > 1050000UL)  || (ref_intern_1v1_uV < 1150000UL) )
        {
            while ( !WriteEeReferenceId() ) {};
            while ( !WriteEeReferenceAvcc() ) {};
            while ( !WriteEeReference1V1() ) {};
            printf_P(PSTR("REF_EXTERN_AVCC saved into eeprom: %lu uV\r\n"), ref_extern_avcc_uV);
            printf_P(PSTR("REF_INTERN_1V1 saved into eeprom: %lu uV\r\n"), ref_intern_1v1_uV);
        }
        else
        { 
            passing = 0; 
            printf_P(PSTR(">>> REF_* for ADC not saved in eeprom.\r\n"));
        }
    }
    digitalWrite(CS_ICP3,LOW);
    _delay_ms(100); // busy-wait delay

    // ICP4 pin is inverted from the plug interface, its Termination should have zero mA. 
    printf_P(PSTR("ICP4 input should be HIGH with 0mA loop current: %d \r\n"), digitalRead(ICP4));
    if (!digitalRead(ICP4)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP4 should be high.\r\n"));
    }

    // enable CS_ICP4
    digitalWrite(CS_ICP4,HIGH);
    _delay_ms(100); // busy-wait delay

    // ICP3_TERM and ICP4_TERM (50Ohm) has CS_ICP4 on it
    float adc0_cs_icp4_v = analogRead(ADC0)*((ref_intern_1v1_uV/1.0E6)/1024.0);
    float adc0_cs_icp4_i = adc0_cs_icp4_v / ICP3AND4_TERM;
    printf_P(PSTR("CS_ICP4 on ICP3AND4 TERM: %1.3f A\r\n"), adc0_cs_icp4_i);
    if (adc0_cs_icp4_i < 0.012) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_ICP4 curr is to low.\r\n"));
    }
    if (adc0_cs_icp4_i > 0.020) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS_ICP4 curr is to high.\r\n"));
    }

    // ICP4 pin is inverted from to the plug interface, and should have 17 mA on the combined Termination
    printf_P(PSTR("ICP4 /w 8mA on termination reads: %d \r\n"), digitalRead(ICP3));
    if (digitalRead(ICP1)) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> ICP4 should be low with 8mA.\r\n"));
    }
    digitalWrite(CS_ICP4,LOW);
    _delay_ms(100); // busy-wait delay
    
    // Input current at no load with 1V1 band-gap referance
    float input_i = analogRead(PWR_I)*((ref_intern_1v1_uV/1.0E6)/1024.0)/(0.068*50.0);
    printf_P(PSTR("PWR_I at no load use INTERNAL_1V1: %1.3f A\r\n"), input_i);
    if (input_i > 0.026) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> Input curr is to high.\r\n"));
        return;
    }
    if (input_i < 0.013) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> Input curr is to low.\r\n"));
    }

    //swap back to the AVCC referance 
    init_ADC_single_conversion(EXTERNAL_AVCC); 
    _delay_ms(100); // busy-wait delay

    // enable CS0 (through red LED)
    digitalWrite(CS0_EN,HIGH);
    _delay_ms(100); // busy-wait delay

    // CS0 drives ICP3 and ICP4 termination which should make a 50 Ohm drop
    float adc0_cs0_v = analogRead(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc0_cs0_i = adc0_cs0_v / ICP3AND4_TERM;
    printf_P(PSTR("CS0 on ICP3&4 TERM: %1.3f A\r\n"), adc0_cs0_i);
    if (adc0_cs0_i < 0.018) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS0 curr is to low.\r\n"));
    }
    if (adc0_cs0_i > 0.026) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS0 curr is to high.\r\n"));
    }
    digitalWrite(CS0_EN,LOW);

    // enable CS1
    digitalWrite(CS1_EN,HIGH);
    _delay_ms(100); // busy-wait delay
    
    // CS1  drives ICP3 and ICP4 termination which should make a 50 Ohm drop
    float adc0_cs1_v = analogRead(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc0_cs1_i = adc0_cs1_v / ICP3AND4_TERM;
    printf_P(PSTR("CS1 on ICP3&4 TERM: %1.3f A\r\n"), adc0_cs1_i);
    if (adc0_cs1_i < 0.018) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS1 curr is to low.\r\n"));
    }
    if (adc0_cs1_i > 0.026) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS1 curr is to high.\r\n"));
    }
    digitalWrite(CS1_EN,LOW);

    // enable CS2
    digitalWrite(CS2_EN,HIGH);
    _delay_ms(100); // busy-wait delay
    
    // CS2  drives ICP3 and ICP4 termination which should make a 50 Ohm drop
    float adc0_cs2_v = analogRead(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc0_cs2_i = adc0_cs2_v / ICP3AND4_TERM;
    printf_P(PSTR("CS2 on ICP3&4 TERM: %1.3f A\r\n"), adc0_cs2_i);
    if (adc0_cs2_i < 0.018) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS2 curr is to low.\r\n"));
    }
    if (adc0_cs2_i > 0.026) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS2 curr is to high.\r\n"));
    }
    digitalWrite(CS2_EN,LOW);

    // enable CS3
    digitalWrite(CS3_EN,HIGH);
    _delay_ms(100); // busy-wait delay
    
    // CS3  drives ICP3 and ICP4 termination which should make a 50 Ohm drop
    float adc0_cs3_v = analogRead(ADC0)*((ref_extern_avcc_uV/1.0E6)/1024.0);
    float adc0_cs3_i = adc0_cs3_v / ICP3AND4_TERM;
    printf_P(PSTR("CS3 on ICP3&4 TERM: %1.3f A\r\n"), adc0_cs3_i);
    if (adc0_cs3_i < 0.018) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS3 curr is to low.\r\n"));
    }
    if (adc0_cs3_i > 0.026) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> CS3 curr is to high.\r\n"));
    }
    digitalWrite(CS3_EN,LOW);

    // Serial One pins loopback, e.g., drive TX1 to test RX1.
    pinMode(TX1,OUTPUT);
    digitalWrite(TX1,HIGH);
    pinMode(RX1,INPUT);
    digitalWrite(RX1,LOW); // turn off the weak pullup
    _delay_ms(50) ; // busy-wait delay
    uint8_t tx1_rd = digitalRead(TX1);
    uint8_t rx1_rd = digitalRead(RX1);
    if (tx1_rd && rx1_rd) 
    { 
        printf_P(PSTR("TX1 loopback to RX1 == HIGH\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX1 %d did not loopback to RX1 %d\r\n"), tx1_rd, rx1_rd);
    }

    digitalWrite(TX1,LOW);
    _delay_ms(50) ; // busy-wait delay
    tx1_rd = digitalRead(TX1);
    rx1_rd = digitalRead(RX1);
    if ( (!tx1_rd) && (!rx1_rd) ) 
    { 
        printf_P(PSTR("TX1 loopback to RX1 == LOW\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX1 %d did not loopback to RX1 %d\r\n"), tx1_rd, rx1_rd);
    }

    // Serial Two pins loopback, e.g., drive TX2 to test RX2.
    pinMode(TX2,OUTPUT);
    digitalWrite(TX2,HIGH);
    pinMode(RX2,INPUT);
    digitalWrite(RX2,LOW); // turn off the weak pullup
    _delay_ms(50) ; // busy-wait delay
    uint8_t tx2_rd = digitalRead(TX2);
    uint8_t rx2_rd = digitalRead(RX2);
    if (tx2_rd && rx2_rd) 
    { 
        printf_P(PSTR("TX2 loopback to RX2 == HIGH\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX2 %d did not loopback to RX2 %d\r\n"), tx2_rd, rx2_rd);
    }

    digitalWrite(TX2,LOW);
    _delay_ms(50) ; // busy-wait delay
    tx2_rd = digitalRead(TX2);
    rx2_rd = digitalRead(RX2);
    if ( (!tx2_rd) && (!rx2_rd) ) 
    { 
        printf_P(PSTR("TX2 loopback to RX2 == LOW\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX2 %d did not loopback to RX2 %d\r\n"), tx2_rd, rx2_rd);
    }

    // XcvrTest
    
    // SMBus from manager needs connected to I2C1 master for testing, it is for write_i2c_block_data and read_i2c_block_data from
    // https://git.kernel.org/pub/scm/utils/i2c-tools/i2c-tools.git/tree/py-smbus/smbusmodule.c
    // write_i2c_block_data sends a command byte and data to the slave 
    smbus_address();

    // SPI loopback at R-Pi header. e.g., drive MISO to test MOSI.
    pinMode(MISO,OUTPUT);
    digitalWrite(MISO,HIGH);
    pinMode(MOSI,INPUT);
    digitalWrite(MOSI,LOW); // turn off the weak pullup on the 324pb side of open drain buffer since it has a 3k pullup resistor 
    _delay_ms(50) ; // busy-wait delay
    uint8_t miso_rd = digitalRead(MISO);
    uint8_t mosi_rd = digitalRead(MOSI);
    if (miso_rd && mosi_rd) 
    { 
        printf_P(PSTR("MISO loopback to MOSI == HIGH\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> MISO %d did not loopback to MOSI %d\r\n"), miso_rd, mosi_rd);
    }

    digitalWrite(MISO,LOW);
    _delay_ms(50) ; // busy-wait delay
    miso_rd = digitalRead(MISO);
    mosi_rd = digitalRead(MOSI);
    if ( (!miso_rd) && (!mosi_rd) ) 
    { 
        printf_P(PSTR("MISO loopback to MOSI == LOW\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> MISO %d did not loopback to MOSI %d\r\n"), miso_rd, mosi_rd);
        printf_P(PSTR(">>> R-Pi POL pin needs 5V for loopback to work\r\n"));
    }

    // My R-Pi Shutdown is on BCM6 (pin31) and that loops back into SCK on the test header
    pinMode(SCK,INPUT);
    digitalWrite(SCK,LOW); // turn off the weak pullup, the RPUpi board has a 3k pullup resistor 
    _delay_ms(50) ; // busy-wait delay
    uint8_t sck_rd = digitalRead(SCK);
    if (sck_rd ) 
    { 
        printf_P(PSTR("SCK with Shutdown loopback == HIGH\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> Shutdown HIGH did not loopback to SCK %d\r\n"), sck_rd);
    }

    // To cause a shutdown I can send the I2C command 5 with data 1
    // note:  this does not monitor the input power of the R-Pi or turn off power to the R-Pi at this time.    
    i2c_shutdown();

    _delay_ms(50) ; // busy-wait delay
    sck_rd = digitalRead(SCK);
    if (!sck_rd) 
    { 
        printf_P(PSTR("SCK with Shutdown loopback == LOW\r\n"));
    }
    else 
    { 
        passing = 0; 
        printf_P(PSTR(">>> Shutdown LOW did not loopback to SCK %d\r\n"), sck_rd);
    }

    // Manager should save the shutdown detected after SHUTDOWN_TIME timer runs (see rpubus_manager_state.h in Remote fw for value)
    _delay_ms(1100) ; // busy-wait delay
    i2c_shutdown_detect();

    // Set test mode which will save trancever control bits HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE
    test_mode_clean = 0;
    delayed_outputs = 0; // each bit selects a output to print
    printf_P(PSTR("\r\n"));
    printf_P(PSTR("Testmode: default trancever control bits\r\n"));
    i2c_testmode_start();
    init_ADC_single_conversion(INTERNAL_1V1);
    _delay_ms(1000) ; // busy-wait delay for input current measurement
    // check xcvr bits after start of testmode. Note printf is done after end of testmode.
    uint8_t xcvrbits_after_testmode_start = 0xE2;
    i2c_testmode_test_xcvrbits(xcvrbits_after_testmode_start);
    float noload_i = analogRead(PWR_I)*((ref_intern_1v1_uV/1.0E6)/1024.0)/(0.068*50.0);

    // End test mode 
    i2c_testmode_end();
    
    // show the delayed test results now that UART is on-line
    if (delayed_outputs & (1<<4))
    {
        printf_P(PSTR(">>> I2C0 cmd 50 write fail, twi_returnCode: %d\r\n"), delayed_data[4]);
    }
    if (delayed_outputs & (1<<5))
    {
        printf_P(PSTR(">>> I2C0 read missing %d bytes \r\n"), delayed_data[5]);
    }
    if (delayed_outputs & (1<<6))
    {
        printf_P(PSTR("Testmode: read  Xcvr cntl bits {50, 0x%X}\r\n"), delayed_data[6]);
    }
    if (delayed_outputs & (1<<7))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_after_testmode_start, delayed_data[7]);
    }
    printf_P(PSTR("PWR_I /w no load using INTERNAL_1V1: %1.3f A\r\n"), noload_i);
    if (noload_i > 0.020) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> no load curr is to high.\r\n"));
    }
    if (noload_i < 0.006) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> no load curr is to low.\r\n"));
    }

    // set CTS low and verify it loops back to RTS
    // control bits  HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE
    test_mode_clean = 0;
    delayed_outputs = 0; // each bit selects a output to print
    printf_P(PSTR("\r\n"));
    printf_P(PSTR("Testmode: nCTS loopback to nRTS\r\n"));
    i2c_testmode_start();
    _delay_ms(50) ; // busy-wait delay

    // set nCTS durring testmode and check the loopback on nRTS. Note printf is done after end of testmode.
    uint8_t xcvrbits_change_cts_bit_low = 0xA2; //0b10100010
    i2c_testmode_set_xcvrbits(xcvrbits_change_cts_bit_low);
    uint8_t xcvrbits_cts_loopback_to_rts = 0x22; //0b00100010
    i2c_testmode_test_xcvrbits(xcvrbits_cts_loopback_to_rts);

    // End test mode 
    i2c_testmode_end();
    
    // show the delayed test results now that UART is on-line
    if (delayed_outputs & (1<<0))
    {
        printf_P(PSTR(">>> I2C0 cmd 51 write fail, twi_returnCode: %d\r\n"), delayed_data[0]);
    }
    if (delayed_outputs & (1<<1))
    {
        printf_P(PSTR(">>> I2C0 read missing %d bytes \r\n"), delayed_data[1]);
    }
    if (delayed_outputs & (1<<2))
    {
        printf_P(PSTR("Testmode: set  Xcvr cntl bits {51, 0x%X}\r\n"), delayed_data[2]);
    }
    if (delayed_outputs & (1<<3))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_change_cts_bit_low, delayed_data[3]);
    }
    if (delayed_outputs & (1<<4))
    {
        printf_P(PSTR(">>> I2C0 cmd 50 write fail, twi_returnCode: %d\r\n"), delayed_data[4]);
    }
    if (delayed_outputs & (1<<5))
    {
        printf_P(PSTR(">>> I2C0 read missing %d bytes \r\n"), delayed_data[5]);
    }
    if (delayed_outputs & (1<<6))
    {
        printf_P(PSTR("Testmode: read  Xcvr cntl bits {50, 0x%X}\r\n"), delayed_data[6]);
    }
    if (delayed_outputs & (1<<7))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_cts_loopback_to_rts, delayed_data[7]);
    }

    // Enable TX driver durring testmode and measure PWR_I.
    test_mode_clean = 0;
    delayed_outputs = 0; // each bit selects a output to print
    printf_P(PSTR("\r\n"));
    printf_P(PSTR("Testmode: Enable TX pair driver\r\n"));
    i2c_testmode_start();
    _delay_ms(50) ; // busy-wait delay
    UCSR0B &= ~( (1<<RXEN0)|(1<<TXEN0) ); // turn off UART 
    pinMode(TX0,OUTPUT);
    digitalWrite(TX0,LOW); // the TX pair will now be driven and load the transceiver 

    // control bits  HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE
    // TX_DE drives the twisted pair from this UART, and TX_nRE drives the line to the host (which is not enabled)
    uint8_t xcvrbits_enable_xtde = 0xF2; //0b11110010
    i2c_testmode_set_xcvrbits(xcvrbits_enable_xtde);
    i2c_testmode_test_xcvrbits(xcvrbits_enable_xtde); // to be clear "...set..." does not verify the setting
    _delay_ms(1000) ; // busy-wait delay
    float load_txde_i = analogRead(PWR_I)*((ref_intern_1v1_uV/1.0E6)/1024.0)/(0.068*50.0);

    // End test mode 
    digitalWrite(TX0,HIGH); // strong pullup
    _delay_ms(10) ; // busy-wait delay
    pinMode(TX0,INPUT); // the TX pair should probably have a weak pullup when UART starts
    UCSR0B |= (1<<RXEN0)|(1<<TXEN0); // turn on UART
    i2c_testmode_end();
    
    // show the delayed test results now that UART is on-line
    if (delayed_outputs & (1<<0))
    {
        printf_P(PSTR(">>> I2C0 cmd 51 write fail, twi_returnCode: %d\r\n"), delayed_data[0]);
    }
    if (delayed_outputs & (1<<1))
    {
        printf_P(PSTR(">>> I2C0 read missing %d bytes \r\n"), delayed_data[1]);
    }
    if (delayed_outputs & (1<<2))
    {
        printf_P(PSTR("Testmode: set  Xcvr cntl bits {51, 0x%X}\r\n"), delayed_data[2]);
    }
    if (delayed_outputs & (1<<3))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_enable_xtde, delayed_data[3]);
    }
    if (delayed_outputs & (1<<4))
    {
        printf_P(PSTR(">>> I2C0 cmd 50 write fail, twi_returnCode: %d\r\n"), delayed_data[4]);
    }
    if (delayed_outputs & (1<<5))
    {
        printf_P(PSTR(">>> I2C0 read missing %d bytes \r\n"), delayed_data[5]);
    }
    if (delayed_outputs & (1<<6))
    {
        printf_P(PSTR("Testmode: read  Xcvr cntl bits {50, 0x%X}\r\n"), delayed_data[6]);
    }
    if (delayed_outputs & (1<<7))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_enable_xtde, delayed_data[7]);
    }
    printf_P(PSTR("PWR_I /w TX pair load: %1.3f A\r\n"), load_txde_i);
    if (load_txde_i > 0.055) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX pair load curr is to high.\r\n"));
    }
    if (load_txde_i < 0.025) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX pair load curr is to low.\r\n"));
    }

    // Enable TX and RX driver durring testmode and measure PWR_I.
    test_mode_clean = 0;
    delayed_outputs = 0; // each bit selects a output to print
    printf_P(PSTR("\r\n"));
    printf_P(PSTR("Testmode: Enable TX & RX(loopback) pair drivers\r\n"));
    i2c_testmode_start();
    _delay_ms(50) ; // busy-wait delay
    UCSR0B &= ~( (1<<RXEN0)|(1<<TXEN0) ); // turn off UART 
    pinMode(TX0,OUTPUT);
    digitalWrite(TX0,LOW); // the TX pair will now be driven and load the transceiver 

    // control bits  HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE
    // TX_DE drives the TX twisted pair from this UART, and TX_nRE drives the line to the host which is a loopback
    // RX_DE drives the RX twisted pair from the loopback, and RX_nRE drives the line to this RX pair.
    uint8_t xcvrbits_enable_xtrx = 0xD1; //0b11010001
    i2c_testmode_set_xcvrbits(xcvrbits_enable_xtrx);
    i2c_testmode_test_xcvrbits(xcvrbits_enable_xtrx); // to be clear "...set..." does not verify the setting
    _delay_ms(1000) ; // busy-wait delay
    float load_txrx_i = analogRead(PWR_I)*((ref_intern_1v1_uV/1.0E6)/1024.0)/(0.068*50.0);
    uint8_t rx_loopback = digitalRead(RX0); 

    // End test mode 
    digitalWrite(TX0,HIGH); // strong pullup
    _delay_ms(10) ; // busy-wait delay
    pinMode(TX0,INPUT); // the TX pair should probably have a weak pullup when UART starts
    UCSR0B |= (1<<RXEN0)|(1<<TXEN0); // turn on UART
    i2c_testmode_end();
    
    // show the delayed test results now that UART is on-line
    if (delayed_outputs & (1<<0))
    {
        printf_P(PSTR(">>> I2C0 cmd 51 write fail, twi_returnCode: %d\r\n"), delayed_data[0]);
    }
    if (delayed_outputs & (1<<1))
    {
        printf_P(PSTR(">>> I2C0 read missing %d bytes \r\n"), delayed_data[1]);
    }
    if (delayed_outputs & (1<<2))
    {
        printf_P(PSTR("Testmode: set  Xcvr cntl bits {51, 0x%X}\r\n"), delayed_data[2]);
    }
    if (delayed_outputs & (1<<3))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_enable_xtrx, delayed_data[3]);
    }
    if (delayed_outputs & (1<<4))
    {
        printf_P(PSTR(">>> I2C0 cmd 50 write fail, twi_returnCode: %d\r\n"), delayed_data[4]);
    }
    if (delayed_outputs & (1<<5))
    {
        printf_P(PSTR(">>> I2C0 read missing %d bytes \r\n"), delayed_data[5]);
    }
    if (delayed_outputs & (1<<6))
    {
        printf_P(PSTR("Testmode: read  Xcvr cntl bits {50, 0x%X}\r\n"), delayed_data[6]);
    }
    if (delayed_outputs & (1<<7))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_enable_xtrx, delayed_data[7]);
    }
    printf_P(PSTR("PWR_I /w TX and RX pairs loaded: %1.3f A\r\n"), load_txrx_i);
    if (load_txrx_i > 0.075) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX and RX pairs load curr are too high.\r\n"));
    }
    if (load_txrx_i < 0.045) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> TX and RX pairs load curr are too low.\r\n"));
    }
    if (rx_loopback)
    {
        passing = 0;
        printf_P(PSTR(">>> RX loopback should be LOW but was HIGH\r\n"));
    }
    else
    {
        printf_P(PSTR("RX loopback checked\r\n"));
    }

   // Enable DTR driver durring testmode and measure PWR_I.
    test_mode_clean = 0;
    delayed_outputs = 0; // each bit selects a output to print
    printf_P(PSTR("\r\n"));
    printf_P(PSTR("Testmode: Enable DTR pair driver\r\n"));
    i2c_testmode_start();
    _delay_ms(50) ; // busy-wait delay

    // control bits  HOST_nRTS:HOST_nCTS:TX_nRE:TX_DE:DTR_nRE:DTR_DE:RX_nRE:RX_DE
    // everything off except DTR_nRE stays on and DTR_DE is turned on
    // when the i2c commad sees DTR_DE set high it turns off the UART and
    // enables the DTR pair driver then pulls the DTR_TXD line low to load the DTR pair.
    uint8_t xcvrbits_enable_dtr = 0xE6; //0b11100110
    i2c_testmode_set_xcvrbits(xcvrbits_enable_dtr);
    i2c_testmode_test_xcvrbits(xcvrbits_enable_dtr); // to be clear "...set..." does not verify the setting
    _delay_ms(1000) ; // busy-wait delay
    float load_dtr_i = analogRead(PWR_I)*((ref_intern_1v1_uV/1.0E6)/1024.0)/(0.068*50.0);

    // End test mode will setup the DTR_TXD line and enable the manager's UART as it recovers
    i2c_testmode_end();
    
    // show the delayed test results now that UART is on-line
    if (delayed_outputs & (1<<0))
    {
        printf_P(PSTR(">>> I2C0 cmd 51 write fail, twi_returnCode: %d\r\n"), delayed_data[0]);
    }
    if (delayed_outputs & (1<<1))
    {
        printf_P(PSTR(">>> I2C0 read missing %d bytes \r\n"), delayed_data[1]);
    }
    if (delayed_outputs & (1<<2))
    {
        printf_P(PSTR("Testmode: set  Xcvr cntl bits {51, 0x%X}\r\n"), delayed_data[2]);
    }
    if (delayed_outputs & (1<<3))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_enable_dtr, delayed_data[3]);
    }
    if (delayed_outputs & (1<<4))
    {
        printf_P(PSTR(">>> I2C0 cmd 50 write fail, twi_returnCode: %d\r\n"), delayed_data[4]);
    }
    if (delayed_outputs & (1<<5))
    {
        printf_P(PSTR(">>> I2C0 read missing %d bytes \r\n"), delayed_data[5]);
    }
    if (delayed_outputs & (1<<6))
    {
        printf_P(PSTR("Testmode: read  Xcvr cntl bits {50, 0x%X}\r\n"), delayed_data[6]);
    }
    if (delayed_outputs & (1<<7))
    {
        printf_P(PSTR(">>> Xcvr cntl bits should be %x but report was %x\r\n"), xcvrbits_enable_dtr, delayed_data[7]);
    }
    printf_P(PSTR("PWR_I /w DTR pair load: %1.3f A\r\n"), load_dtr_i);
    if (load_dtr_i > 0.055) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> DTR load curr is to high.\r\n"));
    }
    if (load_dtr_i < 0.025) 
    { 
        passing = 0; 
        printf_P(PSTR(">>> DTR load curr is to low.\r\n"));
    }

    // final test status
    if (passing)
    {
        printf_P(PSTR("[PASS]\r\n"));
    }
    else
    {
        printf_P(PSTR("[FAIL]\r\n"));
    }
    printf_P(PSTR("\r\n\r\n\r\n"));
}

void led_setup_after_test(void)
{
    setup_pins_off();
    if (passing)
    {
        digitalWrite(CS_ICP1,HIGH);
    }
    else
    {
        digitalWrite(CS0_EN,HIGH);
    }
}

void blink(void)
{
    unsigned long kRuntime = millis() - blink_started_at;
    if ( kRuntime > blink_delay)
    {
        if (passing)
        {
            digitalToggle(CS_ICP1);
        }
        else
        {
            digitalToggle(CS0_EN);
        }
        
        // next toggle 
        blink_started_at += blink_delay; 
    }
}

int main(void)
{
    setup(); 
    
    passing = 1;
    test();
    led_setup_after_test();
    
    while (1) 
    {
        blink();
    }    
}

