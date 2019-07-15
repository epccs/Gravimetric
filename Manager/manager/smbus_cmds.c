/*
smbus_cmds is a i2c RPUBUS manager commands library in the form of a function pointer array  
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
#include <util/delay.h>
#include "../lib/timers.h"
#include "../lib/twi1.h"
#include "../lib/uart.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"
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
void receive_smbus_event(uint8_t* inBytes, int numBytes)
{
    inBytes_to_handle = inBytes;
    smbus_has_numBytes_to_handle = numBytes;
}

// twi1.c has been modified, so it has an interleaved buffer that allows  
// the event to put a copy of the pointer where I can use it outside the ISR.
void handle_smbus_receive(void)
{
    // table of pointers to functions that are selected by the i2c cmmand byte
    static void (*pf[GROUP][MGR_CMDS])(uint8_t*) = 
    {
        {fnRdMgrAddrQuietly, fnWtMgrAddr, fnRdBootldAddr, fnWtBootldAddr, fnRdShtdnDtct, fnWtShtdnDtct, fnRdStatus, fnWtStatus},
        {fnWtArduinMode, fnRdArduinMode, fnNull, fnNull, fnNull, fnNull, fnNull, fnNull},
        {fnNull, fnNull, fnNull, fnNull, fnNull, fnNull, fnNull, fnNull},
        {fnStartTestMode, fnEndTestMode, fnRdXcvrCntlInTestMode, fnWtXcvrCntlInTestMode, fnNull, fnNull, fnNull, fnNull}
    };

    int numBytes = smbus_has_numBytes_to_handle; // place value on stack so it will go away when done.
    smbus_has_numBytes_to_handle = 0; 
    
    uint8_t i;

    // read_i2c_block_data has a single command byte in its data set
    // it will write i2c address, the command* byte, and then cause a repeated start
    // followed by the i2c address (again) and then reading** the data
    // * clock stretching occures during the receive (so handle was done to move this code outside the ISR)
    // ** and the transmit events
    if( (numBytes == 1)  )
    {
        // transmit event is set up to work from an old buffer, the data it needs is in the current buffer. 
        if ( (inBytes_to_handle[0] == smbusBuffer[0]) && (!transmit_data_ready) )
        {
            for(i = 0; i < smbusBufferLength; ++i)
            {
                smbus_oldBuffer[i] = smbusBuffer[i];
            }
            if(i < SMBUS_BUFFER_LENGTH) smbus_oldBuffer[i+1] = 0; // room for null
            smbus_oldBufferLength = smbusBufferLength;
            transmit_data_ready = 1;
        }
        return; // done. Even if command does not match.
    }
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
    return;	
}

// called when SMBus slave has been requested to send data
void transmit_smbus_event(void) 
{
    // For SMBus echo the old data from the previous I2C receive event
    twi1_transmit(smbus_oldBuffer, smbus_oldBufferLength);
    transmit_data_ready = 0;
}