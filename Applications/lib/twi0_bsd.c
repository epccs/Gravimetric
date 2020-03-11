/*
AVR Interrupt-Driven Asynchronous I2C C library 
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

The TWCR register has a 'special' flag TWINT in hardware that clears after writing a 1.
Writing 1 to TWINT casues the hardware flag to be cleared, and reading it will show 0 
until the hardware operation has completed and set it to 1 again. These 'special' flags 
behave differently for write than they do for read.

Multi-Master can lock up when blocking functions are used. Also the master will get a NACK 
if it tries to acceess a slave on the same state machine. Note: an m4809 has two ISR driven
state machines, one for the master and another for the slave but it is not clear to me if
they can share the same IO hardware wihtout locking up. The AVR128DA famly has alternat 
IO hardware, the master ISR can be on one set of hardware while the slave ISR is on the other.
*/

#include <stdbool.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include "io_enum_bsd.h"
#include "twi0_bsd.h"

static volatile uint8_t twi_slave_read_write;

typedef enum TWI_STATE_enum {
    TWI_STATE_READY, // TWI state machine ready for use
    TWI_STATE_MASTER_RECEIVER, // TWI state machine is master receiver
    TWI_STATE_MASTER_TRANSMITTER, // TWI state machine is master transmitter
    TWI_STATE_SLAVE_RECEIVER, // TWI state machine is slave receiver
    TWI_STATE_SLAVE_TRANSMITTER  // TWI state machine is slave transmitter
} TWI_STATE_t;

static volatile TWI_STATE_t twi_MastSlav_RxTx_state;

typedef enum TWI_ACK_enum {
    TWI_ACK,
    TWI_NACK
} TWI_ACK_t;

typedef enum TWI_PROTOCALL_enum {
    TWI_PROTOCALL_STOP = 0x01,
    TWI_PROTOCALL_REPEATEDSTART = 0x02
} TWI_PROTOCALL_t;

static volatile TWI_PROTOCALL_t twi_protocall; 

typedef enum TWI_ERROR_enum {
    TWI_ERROR_ILLEGAL = TW_BUS_ERROR, // illegal start or stop condition
    TWI_ERROR_SLAVE_ADDR_NACK = TW_MT_SLA_NACK, // SLA+W transmitted, NACK received 
    TWI_ERROR_DATA_NACK = TW_MT_DATA_NACK, // data transmitted, NACK received
    TWI_ERROR_ARBITRATION_LOST = TW_MT_ARB_LOST, // arbitration lost in SLA+W or data
    TWI_ERROR_NONE = 0xFF // No errors
} TWI_ERROR_t;


static volatile TWI_ERROR_t twi_error;

// used to initalize the slave Transmit functions in case they are not used.
void transmit_default(void)
{
    // In a real callback, the data to send needs to be copied from a local buffer.
    // uint8_t return_code = twi0_fillSlaveTxBuffer(localBuffer, localBufferLength);
    return;
}

typedef void (*PointerToTransmit)(void);

// used to initalize the slave Receive functions in case they are not used.
void receive_default(uint8_t *data, int length)
{
    // In a real callback, the data to send needs to be copied to a local buffer.
    // 
    // for(int i = 0; i < length; ++i)
    // {
    //     localBuffer[i] = data[i];
    // }
    //
    // the receive event happens once after the I2C stop or repeated-start
    //
    // repeated-start is usd for atomic bus operation e.g. prevents others from using bus 
    return;
}

typedef void (*PointerToReceive)(uint8_t*, int);

static PointerToTransmit twi0_onSlaveTx = transmit_default;
static PointerToReceive twi0_onSlaveRx = receive_default;

static uint8_t masterBuffer[TWI0_BUFFER_LENGTH];
static volatile uint8_t masterBufferIndex;
static volatile uint8_t masterBufferLength;

static uint8_t slaveTxBuffer[TWI0_BUFFER_LENGTH];
static volatile uint8_t slaveTxBufferIndex;
static volatile uint8_t slaveTxBufferLength;

// enable interleaving buffer for R-Pi Zero in header
static uint8_t slaveRxBufferA[TWI0_BUFFER_LENGTH];
#ifdef TWI0_SLAVE_RX_BUFFER_INTERLEAVING
static uint8_t slaveRxBufferB[TWI0_BUFFER_LENGTH];
#endif

static uint8_t *slaveRxBuffer;
static volatile uint8_t slaveRxBufferIndex;

// STOP condition
void stop_condition(void)
{
    TWCR0 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWSTO);

    // An ISR event does not happen for a stop condition
    while(TWCR0 & (1<<TWSTO))
    {
        continue;
    }

    twi_MastSlav_RxTx_state = TWI_STATE_READY;
}

// ready the bus but do not STOP it
void ready_bus(void)
{
    TWCR0 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA) | (1<<TWINT);
    twi_MastSlav_RxTx_state = TWI_STATE_READY;
}

// ACK or NACK
void acknowledge(TWI_ACK_t ack)
{
    if(ack == TWI_ACK)
    {
        TWCR0 = (1<<TWEN) | (1<<TWIE) | (1<<TWINT) | (1<<TWEA);
    }
    else
    {
        TWCR0 = (1<<TWEN) | (1<<TWIE) | (1<<TWINT);
    }
}

ISR(TWI0_vect)
{
    switch(TWSR0 & TW_STATUS_MASK) // TW_STATUS can be used for part with one TWI
    {
        // Illegal start or stop condition
        case TW_BUS_ERROR:
            twi_error = TWI_ERROR_ILLEGAL;
            stop_condition();
            break;

        // Master start condition transmitted
        case TW_START:
        case TW_REP_START:
            TWDR0 = twi_slave_read_write;
            acknowledge(TWI_ACK);
            break;

        // Master Transmitter SLA+W transmitted, ACK received
        case TW_MT_SLA_ACK:
        case TW_MT_DATA_ACK:

            if(masterBufferIndex < masterBufferLength)
            {
                TWDR0 = masterBuffer[masterBufferIndex++];
                acknowledge(TWI_ACK);
            }
            else
            {
                if (twi_protocall & TWI_PROTOCALL_STOP)
                    stop_condition();
                else 
                {
                    // Generate the START but set flag for the next transaction.
                    twi_protocall |= TWI_PROTOCALL_REPEATEDSTART;
                    TWCR0 = (1<<TWINT) | (1<<TWSTA)| (1<<TWEN) ;
                    twi_MastSlav_RxTx_state = TWI_STATE_READY;
                }
            }
            break;

        // SLA+W transmitted, NACK received
        case TW_MT_SLA_NACK:
            twi_error = TWI_ERROR_SLAVE_ADDR_NACK; 
            stop_condition();
            break;

        // data transmitted, NACK received
        case TW_MT_DATA_NACK:
            twi_error = TWI_ERROR_DATA_NACK;
            stop_condition();
            break;

        // arbitration lost in SLA+W or data
        case TW_MT_ARB_LOST: // same as TW_MR_ARB_LOST
            twi_error = TWI_ERROR_ARBITRATION_LOST;
            ready_bus();
            break;

        // Slave SLA+R transmitted, NACK received
        case TW_MR_SLA_NACK:
            stop_condition();
            break;

        // Master data received, ACK returned
        case TW_MR_DATA_ACK:
            masterBuffer[masterBufferIndex++] = TWDR0;
        case TW_MR_SLA_ACK:  // SLA+R transmitted, ACK received
            if(masterBufferIndex < masterBufferLength)
            {
                acknowledge(TWI_ACK);
            }
            else
            {
                acknowledge(TWI_NACK);
            }
            break;
        
        // Master data received, NACK returned
        case TW_MR_DATA_NACK:
            masterBuffer[masterBufferIndex++] = TWDR0;
            if (twi_protocall & TWI_PROTOCALL_STOP)
                stop_condition();
            else 
            {
                // Generate the START but set flag for the next transaction.
                twi_protocall |= TWI_PROTOCALL_REPEATEDSTART;
                TWCR0 = (1<<TWINT) | (1<<TWSTA)| (1<<TWEN) ;
                twi_MastSlav_RxTx_state = TWI_STATE_READY;
            }    
            break;

        // Slave SLA+W received, ACK returned
        case TW_SR_SLA_ACK:
        case TW_SR_GCALL_ACK:
        case TW_SR_ARB_LOST_SLA_ACK: 
        case TW_SR_ARB_LOST_GCALL_ACK:
            twi_MastSlav_RxTx_state = TWI_STATE_SLAVE_RECEIVER;
            slaveRxBufferIndex = 0;
            acknowledge(TWI_ACK);
            break;

        // Slave data received, ACK returned
        case TW_SR_DATA_ACK: 
        case TW_SR_GCALL_DATA_ACK: 
            if(slaveRxBufferIndex < TWI0_BUFFER_LENGTH)
            {
                slaveRxBuffer[slaveRxBufferIndex++] = TWDR0;
                acknowledge(TWI_ACK);
            }
            else
            {
                acknowledge(TWI_NACK);
            }
            break;

        // Slave Data received, return NACK
        case TW_SR_DATA_NACK:       
        case TW_SR_GCALL_DATA_NACK: 
            acknowledge(TWI_NACK);
            break;

        // Slave stop or repeated start condition received while selected
        case TW_SR_STOP:
            ready_bus();
            if(slaveRxBufferIndex < TWI0_BUFFER_LENGTH)
            {
                slaveRxBuffer[slaveRxBufferIndex] = '\0';
            }
            twi0_onSlaveRx(slaveRxBuffer, slaveRxBufferIndex);
            #ifdef TWI0_SLAVE_RX_BUFFER_INTERLEAVING
            if (slaveRxBuffer == slaveRxBufferA) 
            {
                slaveRxBuffer = slaveRxBufferB;
            }
            else
            {
                slaveRxBuffer = slaveRxBufferA;
            }
            #endif
            slaveRxBufferIndex = 0;
            break;

        // Slave SLA+R received, ACK returned
        case TW_ST_SLA_ACK: 
        case TW_ST_ARB_LOST_SLA_ACK:
            twi_MastSlav_RxTx_state = TWI_STATE_SLAVE_TRANSMITTER;
            slaveTxBufferIndex = 0;
            slaveTxBufferLength = 0; 
            twi0_onSlaveTx(); // use twi0_fillSlaveTxBuffer(bytes, length) in callback
            if(0 == slaveTxBufferLength) // default callback does not set this
            {
                slaveTxBufferLength = 1;
                slaveTxBuffer[0] = 0x00;
            }
        case TW_ST_DATA_ACK:
            TWDR0 = slaveTxBuffer[slaveTxBufferIndex++];
            if(slaveTxBufferIndex < slaveTxBufferLength)
            {
                acknowledge(TWI_ACK);
            }
            else
            {
                acknowledge(TWI_NACK);
            }
            break;

        // Slave data transmitted, ACK or NACK received
        case TW_ST_LAST_DATA:
        case TW_ST_DATA_NACK:
            acknowledge(TWI_ACK);
            twi_MastSlav_RxTx_state = TWI_STATE_READY;
            break;

        // Some ISR events get ignored
        case TW_NO_INFO: 
            break;
    }
}

/*************** PUBLIC ***********************************/

// Initialize TWI0 module (bitrate, pull-up)
// if bitrate is 0 then disable twi, a normal bitrate is 100000UL, 
void twi0_init(uint32_t bitrate, TWI0_PINS_t pull_up)
{
    if (bitrate == 0)
    {
        // disable twi module, acks, and twi interrupt
        TWCR0 &= ~((1<<TWEN) | (1<<TWIE) | (1<<TWEA));

        // deactivate internal pullups for twi.
        ioWrite(MCU_IO_SCL0, LOGIC_LEVEL_LOW); // PORTC &= ~(1 << PORTC0) disable the pull-up
        ioWrite(MCU_IO_SDA0, LOGIC_LEVEL_LOW); // PORTC &= ~(1 << PORTC1)
    }
    else
    {
        // start with interleaving buffer A (B is an optional buffer if it is enabled)
        slaveRxBuffer = slaveRxBufferA;

        // initialize state machine
        twi_MastSlav_RxTx_state = TWI_STATE_READY;
        twi_protocall = TWI_PROTOCALL_STOP & ~TWI_PROTOCALL_REPEATEDSTART;

        ioDir(MCU_IO_SCL0, DIRECTION_INPUT); // DDRC &= ~(1 << DDC0)
        ioDir(MCU_IO_SDA0, DIRECTION_INPUT); // DDRC &= ~(1 << DDC1)

        // weak pullup pull-up.
        if (pull_up == TWI0_PINS_PULLUP)
        {
            ioWrite(MCU_IO_SCL0, LOGIC_LEVEL_HIGH); // PORTC |= (1 << PORTC0)
            ioWrite(MCU_IO_SDA0, LOGIC_LEVEL_HIGH); // PORTC |= (1 << PORTC1)
        }

        // initialize TWPS[0:1]=0 for a prescaler = 1
        TWSR0 &= ~((1<<TWPS0));
        TWSR0 &= ~((1<<TWPS1));

        // bitrate = (F_CPU)/(16+(2*TWBR0*prescaler))
        // TWBR0 = ((F_CPU) - bitrate*16)/(bitrate*2*prescaler)
        // TWBR0 = ((F_CPU/bitrate) - 16)/(2*prescaler)
        TWBR0 = ((F_CPU / bitrate) - 16) / 2; //I2C default is 100000L
        // At 16MHz TWBR0 is 72 which is saved in TWDR0 and 
        // the bitrate is perfect (F_CPU)/(16+(2*TWBR0*1)) = 100000

        // enable twi module, acks, and twi interrupt
        TWCR0 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA);
    }
}

// TWI Asynchronous Write Transaction.
// 0 .. Transaction started, check status for success
// 1 .. to much data, it did not fit in buffer
// 2 .. TWI state machine not ready for use
TWI0_WRT_t twi0_masterAsyncWrite(uint8_t slave_address, uint8_t *write_data, uint8_t bytes_to_write, uint8_t send_stop)
{
    uint8_t i;

    if(bytes_to_write > TWI0_BUFFER_LENGTH)
    {
        return TWI0_WRT_TO_MUCH_DATA;
    }
    else
    {    
        if(twi_MastSlav_RxTx_state != TWI_STATE_READY)
        {
            return TWI0_WRT_NOT_READY;
        }
        else // do TWI0_WRT_TRANSACTION_STARTED
        {
            twi_MastSlav_RxTx_state = TWI_STATE_MASTER_TRANSMITTER;
            if (send_stop)
            {
                twi_protocall |= TWI_PROTOCALL_STOP;
            }
            else
            {
                twi_protocall &= ~TWI_PROTOCALL_STOP;
            }
            
            twi_error = TWI_ERROR_NONE;
            masterBufferIndex = 0;
            masterBufferLength = bytes_to_write;
            for(i = 0; i < bytes_to_write; ++i)
            {
                masterBuffer[i] = write_data[i];
            }
        
            // build SLA+W, slave device address + write bit
            twi_slave_read_write = slave_address << 1;
            twi_slave_read_write += TW_WRITE;
        
            // Check if ISR has done the I2C START
            if (twi_protocall & TWI_PROTOCALL_REPEATEDSTART) 
                {
                uint8_t local_twi_protocall = twi_protocall;
                local_twi_protocall &= ~TWI_PROTOCALL_REPEATEDSTART;
                twi_protocall = local_twi_protocall;
                do 
                {
                    TWDR0 = twi_slave_read_write;
                } while(TWCR0 & (1<<TWWC));

                // enable INTs, skip START
                TWCR0 = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE);
            }
            else
            {
                // enable INTs and START
                TWCR0 = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE) | (1<<TWSTA);
            }
            return TWI0_WRT_TRANSACTION_STARTED;
        }
    }
}

// TWI master write transaction status.
TWI0_WRT_STAT_t twi0_masterAsyncWrite_status(void)
{
    if (TWI_STATE_MASTER_TRANSMITTER == twi_MastSlav_RxTx_state)
        return TWI0_WRT_STAT_BUSY;
    else if (TWI_ERROR_NONE == twi_error)
        return TWI0_WRT_STAT_SUCCESS;
    else if (TWI_ERROR_SLAVE_ADDR_NACK == twi_error) 
        return TWI0_WRT_STAT_ADDR_NACK;
    else if (TWI_ERROR_DATA_NACK == twi_error) 
        return TWI0_WRT_STAT_DATA_NACK;
    else if (TWI_ERROR_ILLEGAL == twi_error) 
        return TWI0_WRT_STAT_ILLEGAL;
    else 
        return 5; // can not happen
}

// TWI write busy-wait transaction, do not use with multi-master.
// 0 .. success
// *1 .. length to long for buffer
//      * the busy status is replaced by buffer error 
// 2 .. address send, NACK received
// 3 .. data send, NACK received
// 4 .. illegal start or stop condition
uint8_t twi0_masterBlockingWrite(uint8_t slave_address, uint8_t* write_data, uint8_t bytes_to_write, uint8_t send_stop)
{
    TWI0_WRT_t twi_state_machine = twi0_masterAsyncWrite(slave_address, write_data, bytes_to_write, send_stop);
    if (twi_state_machine == TWI0_WRT_NOT_READY) 
    {
        return 1; // to much data so it was ignored
    }
    else
    {
        // TWI state machine not ready, so wait
        while(twi_state_machine == TWI0_WRT_NOT_READY)
        {
            twi_state_machine = twi0_masterAsyncWrite(slave_address, write_data, bytes_to_write, send_stop);
        }
        TWI0_WRT_STAT_t status = TWI0_WRT_STAT_BUSY; // busy
        // wait for anything except busy
        while(status == TWI0_WRT_STAT_BUSY)
        {
            status = twi0_masterAsyncWrite_status();
        }
        return status;
    }
}

// TWI Asynchronous Read Transaction.
// 0 .. data fit in buffer, check twi0_masterAsyncRead_bytesRead for when it is done
// 1 .. data will not fit in the buffer, request ignored
// 2 .. todo: TWI state machine not ready for use
TWI0_RD_t twi0_masterAsyncRead(uint8_t slave_address, uint8_t bytes_to_read, uint8_t send_stop)
{
    if(bytes_to_read > TWI0_BUFFER_LENGTH)
    {
        return TWI0_RD_TO_MUCH_DATA;
    }
    else
    {
        if (TWI_STATE_READY != twi_MastSlav_RxTx_state)
        {
            return TWI0_RD_NOT_READY;
        }
        else
        {
            twi_MastSlav_RxTx_state = TWI_STATE_MASTER_RECEIVER;
            if (send_stop)
            {
                twi_protocall |= TWI_PROTOCALL_STOP;
            }
            else
            {
                twi_protocall &= ~TWI_PROTOCALL_STOP;
            }
            twi_error = TWI_ERROR_NONE;

            masterBufferIndex = 0;

            // set NACK when the _next_ to last byte received. 
            masterBufferLength = bytes_to_read-1; 

            // build SLA+R, slave device address + r bit
            twi_slave_read_write = slave_address << 1;
            twi_slave_read_write += TW_READ;


            // Check if ISR has done the I2C START
            if (twi_protocall & TWI_PROTOCALL_REPEATEDSTART)
            {
                uint8_t local_twi_protocall = twi_protocall;
                local_twi_protocall &= ~TWI_PROTOCALL_REPEATEDSTART;
                twi_protocall = local_twi_protocall;
                do 
                {
                    TWDR0 = twi_slave_read_write;
                } while(TWCR0 & (1<<TWWC));
                TWCR0 = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE);	// enable INTs, but not START
            }
            else
            {
                // send start condition
                TWCR0 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWSTA);
            }
            return TWI0_RD_TRANSACTION_STARTED;
        }
    }
}

// TWI master read transaction status.
// Output   0 .. twi busy, read operation not complete
//      1..32 .. number of bytes read
uint8_t twi0_masterAsyncRead_bytesRead(uint8_t *read_data)
{
    if (twi_MastSlav_RxTx_state == TWI_STATE_MASTER_RECEIVER)
    {
        return 0;
    }

    uint8_t bytes_read = masterBufferLength+1;
    if (masterBufferIndex < bytes_read )
    {
        bytes_read = masterBufferIndex;
    }

    uint8_t i;
    for(i = 0; i < bytes_read; ++i)
    {
        read_data[i] = masterBuffer[i];
    }
	
    return bytes_read;
}

// TWI read busy-wait transaction, do not use with multi-master.
// 0 returns if requeste for data will not fit in buffer
// 1..32 returns the number of bytes
uint8_t twi0_masterBlockingRead(uint8_t slave_address, uint8_t* read_data, uint8_t bytes_to_read, uint8_t send_stop)
{
    TWI0_RD_t twi_state_machine = twi0_masterAsyncRead(slave_address, bytes_to_read, send_stop);
    if (twi_state_machine == TWI0_RD_TO_MUCH_DATA)
    {
        return 0; // data will not fit in the buffer, request ignored
    }
    else
    {
        // TWI state machine not ready, so wait
        while(twi_state_machine == TWI0_RD_NOT_READY)
        {
            twi_state_machine = twi0_masterAsyncRead(slave_address, bytes_to_read, send_stop);
        }
        // wait for read operation to complete (a non-zero is the bytes read, zero is busy)
        uint8_t bytes_read = 0;
        while(!bytes_read)
        {
            bytes_read = twi0_masterAsyncRead_bytesRead(read_data);
        }
        return bytes_read;
    }
}

//  fill slaveTxBuffer using callback
uint8_t twi0_fillSlaveTxBuffer(const uint8_t* slave_data, uint8_t bytes_to_send)
{
    if(TWI0_BUFFER_LENGTH < bytes_to_send)
    {
        return 1;
    }
  
    if(TWI_STATE_SLAVE_TRANSMITTER != twi_MastSlav_RxTx_state)
    {
        return 2;
    }
  
    slaveTxBufferLength = bytes_to_send;
    for(uint8_t i = 0; i < bytes_to_send; ++i)
    {
        slaveTxBuffer[i] = slave_data[i];
    }
  
    return 0;
}

// record callback to use durring a slave read operation
void twi0_registerSlaveRxCallback( void (*function)(uint8_t*, int) )
{
    twi0_onSlaveRx = function;
}

// record callback to use before a slave write operation
void twi0_registerSlaveTxCallback( void (*function)(void) )
{
    twi0_onSlaveTx = function;
}
