/*
Manager callback functions are done over the i2c interface between application controller and manager
Copyright (c) 2020 Ronald S. Sutherland

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

#include <stdio.h>
#include <string.h>
#include "../lib/twi0_bsd.h"
#include "../lib/io_enum_bsd.h"
#include "i2c_callback.h"

// 1 .. length to long for buffer 
// 2 .. address send, NACK received 
// 3 .. data send, NACK received 
// 4 .. other twi error (e.g., lost bus arbitration, bus error) 
// 5 .. read does not match length
// 6 .. bad command
// 7 .. prevent sending bad data
uint8_t twi_errorCode;

// largest I2C transaction with manager so far is six bytes.
#define MAX_CMD_SIZE 8

uint8_t i2c_address_;
uint8_t txBuffer_[MAX_CMD_SIZE];
uint8_t bytes_to_write_; // master wrties bytes to slave (you may want to zero the last byte txBuffer array)
uint8_t rxBuffer_[MAX_CMD_SIZE];
uint8_t bytes_to_read_; // master reads bytes from slave (you may want to zero the rxBuffer array)
uint8_t i2c_address_; // master address this slave

// command is used to send a byte from manager
#define ADDRESS_CMD {0x01,0x00}
#define ADDRESS_CMD_SIZE 2


// cycle the twi state machine on both the master and slave(s)
void i2c_ping(uint8_t i2c_address, TWI0_LOOP_STATE_t *loop_state)
{ 
    // ping I2C for an RPU bus manager 
    uint8_t data = 0;
    uint8_t length = 0;
    for (uint8_t i =0;1; i++) // try a few times, it is slower starting after power up.
    {
        twi_errorCode = twi0_masterBlockingWrite(i2c_address, &data, length, TWI0_PROTOCALL_STOP); 
        if (twi_errorCode == 0) break; // error free code
        if (i>5) return; // give up after 5 trys
    }
    return; 
}

// command is used to send a byte from manager to application
// i2c_address is the slave address for callback to place the data
// command allows the slave address to be used for multiple data (e.g., events or state machine values)
void i2c_callback(uint8_t i2c_address, uint8_t command, uint8_t data, TWI0_LOOP_STATE_t *loop_state)
{ 
    // if not in INTI state ignore 
    if (*loop_state == TWI0_LOOP_STATE_INIT)
    {
        i2c_address_ = i2c_address;
        bytes_to_write_ = ADDRESS_CMD_SIZE;
        txBuffer_[0] = command; // replace the command byte
        txBuffer_[1] = data; // replace the select byte
        txBuffer_[2] = 0;
        bytes_to_read_ = ADDRESS_CMD_SIZE;
        rxBuffer_[0] = 0;
        rxBuffer_[1] = 0;
        rxBuffer_[2] = 0;
        ioWrite(MCU_IO_MGR_SCK_LED, LOGIC_LEVEL_LOW); // LED ON
        *loop_state = TWI0_LOOP_STATE_ASYNC_WRT; // set write state
    }
}

// wait for TWI to finish
uint8_t i2c_callback_waiting(TWI0_LOOP_STATE_t *loop_state)
{ 
    uint8_t done = 1; 
    if (*loop_state > TWI0_LOOP_STATE_INIT)
    {
        uint8_t bytes_read = twi0_masterWriteRead(i2c_address_, txBuffer_, bytes_to_write_, rxBuffer_, bytes_to_read_, loop_state);
        if( (*loop_state == TWI0_LOOP_STATE_DONE) )
        {
            // twi0_masterWriteRead error code is in bits 5..7
            if(bytes_read & 0xE0)
            {
                twi_errorCode = bytes_read>>5;
            }
            ioWrite(MCU_IO_MGR_SCK_LED, LOGIC_LEVEL_HIGH); // LED OFF
            *loop_state = TWI0_LOOP_STATE_RAW;
            done = 0;
        }
    }
    else // INIT and DONE should not occure, but just in case make them RAW
    {
        *loop_state = TWI0_LOOP_STATE_RAW;
        done = 0;
    }
    
    return done;
}