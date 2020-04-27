/*
smbus_cmds is a i2c RPUBUS manager commands library in the form of a function pointer array  
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
#include <avr/io.h>
#include <util/delay.h>
#include "../lib/twi1_bsd.h"
#include "../lib/uart0_bsd.h"
#include "rpubus_manager_state.h"
#include "i2c_cmds.h"
#include "smbus_cmds.h"

uint8_t smbusBuffer[SMBUS_BUFFER_LENGTH];
uint8_t smbusBufferLength = 0;
uint8_t smbus_oldBuffer[SMBUS_BUFFER_LENGTH]; //i2c1_old is for SMBus
uint8_t smbus_oldBufferLength = 0;
uint8_t transmit_data_ready = 0;
uint8_t* inBytes_to_handle;
int smbus_has_numBytes_to_handle;



// called when SMBus slave has received data
// minimize clock streatching for R-Pi. 
// use smbus_has_numBytes_to_handle as smbus flag to run handle routine outside ISR
void receive_smbus_event(uint8_t* inBytes, uint8_t numBytes)
{
    inBytes_to_handle = inBytes;
    smbus_has_numBytes_to_handle = numBytes;
}

// twi1.c has been modified, so it has an interleaved buffer that allows  
// the event to put a copy of the pointer where I can use it outside the ISR.
void handle_smbus_receive(void)
{
    if (smbus_has_numBytes_to_handle)
    {
        // table of pointers to functions that are selected by the i2c cmmand byte
        static void (*pf[GROUP][MGR_CMDS])(uint8_t*) = 
        {
            {fnMgrAddrQuietly, fnNull, fnBootldAddr, fnArduinMode, fnShtdnDtct, fnNull, fnStatus, fnNull},
            {fnNull, fnNull, fnBatChrgLowLim, fnBatChrgHighLim, fnRdBatChrgTime, fnMorningThreshold, fnEveningThreshold, fnDayNightState},
            {fnAnalogRead, fnCalibrationRead, fnNull, fnNull, fnRdTimedAccum, fnNull, fnReferance, fnNull},
            {fnStartTestMode, fnEndTestMode, fnRdXcvrCntlInTestMode, fnWtXcvrCntlInTestMode, fnMorningDebounce, fnEveningDebounce, fnDayNightTimer, fnNull}
        };

        int numBytes = smbus_has_numBytes_to_handle; // place value on stack so it will go away when done.
        smbus_has_numBytes_to_handle = 0; 
        
        uint8_t i;

        // write_i2c_block_data has a command byte followed by data
        // e.g., start+>addr+>command+>data+stop
        // read_i2c_block_data has a single command byte that is sent befor reading
        // e.g., start+>addr+>command+repeated-start+>addr+<data+stop
        // the slave receive event has saved a pointer to the interleaving buffer with the receive >data 
        
        // a read operation has one byte, and it is the command
        if( (numBytes == 1)  )
        {
            // check the command in previous write operation matchs this read operation
            // transmit event is set up to work from an old buffer, the data it needs is in the current buffer. 
            if ( (inBytes_to_handle[0] == smbusBuffer[0]) ) // To not allow consecutive writes add " && (!transmit_data_ready)"
            {
                for(i = 0; i < smbusBufferLength; ++i)
                {
                    smbus_oldBuffer[i] = smbusBuffer[i];
                }
                if(i < SMBUS_BUFFER_LENGTH) smbus_oldBuffer[i+1] = 0; // room for null
                smbus_oldBufferLength = smbusBufferLength;
                transmit_data_ready = 1;
            }
            return; // done. Even if command did not match.
        }

        // a write operation has a command plus >data, and can follow each other
        for(i = 0; i < numBytes; ++i)
        {
            smbusBuffer[i] = inBytes_to_handle[i];    
        }
        if(i < SMBUS_BUFFER_LENGTH) smbusBuffer[i+1] = 0; // room for null
        smbusBufferLength = numBytes;

        // an read_i2c_block_data has a command byte 
        if( !(smbusBufferLength > 0) ) 
        {
            smbusBuffer[0] = 0xFF; // error code for small size.
            return; // not valid, do nothing just echo an error code.
        }

        // mask the group bits (4..7) so they are alone then roll those bits to the left so they can be used as an index.
        uint8_t group;
        group = (smbusBuffer[0] & 0xF0) >> 4;
        if(group >= GROUP) 
        {
            smbusBuffer[0] = 0xFE; // error code for bad group.
            return; 
        }

        // mask the command bits (0..3) so they can be used as an index.
        uint8_t command;
        command = smbusBuffer[0] & 0x0F;
        if(command >= MGR_CMDS) 
        {
            smbusBuffer[0] = 0xFD; // error code for bad command.
            return; // not valid, do nothing but echo error code.
        }

        // Call the i2c command function and return
        (* pf[group][command])(smbusBuffer);
    }
    return;
}

// called when SMBus slave has been requested to send data
void transmit_smbus_event(void) 
{
    // For SMBus echo the old data from the previous I2C receive event
    twi1_fillSlaveTxBuffer(smbus_oldBuffer, smbus_oldBufferLength);
    transmit_data_ready = 0;
}