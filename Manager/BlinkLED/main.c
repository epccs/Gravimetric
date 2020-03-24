/* Blink LED plus toggle from SMBus
Copyright (C) 2020 Ronald Sutherland

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

#include <util/delay.h>
#include <avr/io.h>
#include "../lib/timers_bsd.h"
#include "../lib/twi1_bsd.h"
#include "../lib/io_enum_bsd.h"

static uint8_t smbusBuffer[TWI1_BUFFER_LENGTH];
static uint8_t smbusBufferLength = 0;
static uint8_t smbus_readBlock[TWI1_BUFFER_LENGTH];
static uint8_t smbus_readBlockLength = 0;
volatile static uint8_t* twi1_receiveByteArray_to_handle;
volatile static uint8_t smbus_has_numBytes_to_handle;

static unsigned long blink_started_at;

static uint8_t i2c_toggle;

// called when I2C1 slave has received data
// An R-Pi Zero needs to limit clock stretching so an interleaved
// buffer is used and the callback saves a pointer to the buffer
// that was loaded.
void twi1_receive_event(uint8_t* inBytes, uint8_t numBytes) 
{
    twi1_receiveByteArray_to_handle = inBytes;
    smbus_has_numBytes_to_handle = numBytes;
}

// twi1_bsd.c is set to use an interleaved buffer that allows  
// the event to save a copy of the pointer so I can use it outside the ISR.
// write_i2c_block_data has a command byte followed by data
// e.g., start+>addr+>command+>data+stop
// read_i2c_block_data has a single command byte that is sent befor reading
// e.g., start+>addr+>command+repeated-start+>addr+<data+stop
// the slave receive event has saved a pointer to the interleaving buffer with the receive >data 
void handle_smbus_receive(void)
{
    if (smbus_has_numBytes_to_handle)
    {
        uint8_t local_numBytes = smbus_has_numBytes_to_handle; // local
        smbus_has_numBytes_to_handle = 0; 
        uint8_t* local_receiveByteArray = (uint8_t*) twi1_receiveByteArray_to_handle; 
        uint8_t i;
        
        // a read operation has one command byte befor a repeated-start
        // so there should be enough time to prep the readBlock befor the callback needs it
        if(local_numBytes == 1)  
        {
            // The command in previous write operation should have processed the data
            // and placed the results in the buffer.
            if ( (local_receiveByteArray[0] == smbusBuffer[0]) ) // command should match for this to be a valid read operation
            {
                // copy the previous write operation data into the buffer that will be sent back durring the next transmit event
                for(i = 0; i < smbusBufferLength; ++i)
                {
                    smbus_readBlock[i] = smbusBuffer[i];
                }
                if(i < TWI1_BUFFER_LENGTH) smbus_readBlock[i+1] = 0; // room for null
                smbus_readBlockLength = smbusBufferLength;
            }
            return; // done. Even if command did not match.
        }

        // skip commands without data, all commands do the same thing since this is a test/example
        if (local_numBytes > 1)
        {
            // every i2c command will toggle the LED
            i2c_toggle = !i2c_toggle;

            // save the command byte
            smbusBuffer[0] = local_receiveByteArray[0];
                
            // and invert the data sent, just to see it did somthing
            for(i = 1; i < local_numBytes; ++i)
            {
                smbusBuffer[i] = ~local_receiveByteArray[i];   
            }

            // save the write block size (but not if it is a read block command)
            smbusBufferLength = local_numBytes;
        }
    }
    return;
}

// called when I2C slave has been requested to send data
// Clock streatching should have been enabled befor this event was called
void twi1_transmit_event(void) 
{
    // For SMBus echo the readBlock which the the previous I2C write operation made ready
    twi1_fillSlaveTxBuffer(smbus_readBlock, smbus_readBlockLength);
}

int main(void)
{
    i2c_toggle = 1;
    ioDir(MCU_IO_MGR_SCK_LED, DIRECTION_OUTPUT);
    ioWrite(MCU_IO_MGR_SCK_LED, LOGIC_LEVEL_LOW); // turn the LED on by sinking current
    ioDir(MCU_IO_MGR_nSS, DIRECTION_INPUT); // nSS goes to the controller nRESET thru a 74LVC07 buffer
    ioWrite(MCU_IO_MGR_nSS, LOGIC_LEVEL_HIGH); // the manager needs a pull-up or the controller may be held in reset

    initTimers(); //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.

    // twi1 is set up with interleaved buffer for use with SMbus bus master that does not like clock-stretching (e.g., R-Pi Zero) 
    twi1_slaveAddress(0x2A); // #define I2C1_ADDRESS 0x2A
    twi1_registerSlaveTxCallback(twi1_transmit_event); // called when SMBus slave has been requested to send data
    twi1_registerSlaveRxCallback(twi1_receive_event); // called when SMBus slave has received data
    twi1_init(100000UL, TWI1_PINS_FLOATING); // do not use internal pull-up a Raspberry Pi has them on board
    
    sei(); // Enable global interrupts to start TIMER0 and twi
    
    blink_started_at = milliseconds();

    while (1) 
    {
        // check for smbus (twi1) receive in main loop 
        handle_smbus_receive();

        if (elapsed(&blink_started_at) > 1000) 
        {
            if (i2c_toggle) ioToggle(MCU_IO_MGR_SCK_LED);
            
            // next toggle 
            blink_started_at += 1000; 
        }
    }    
}

