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
#include "twi1_bsd.h"

static volatile uint8_t twi1_slave_read_write;

typedef enum TWI_STATE_enum {
    TWI_STATE_READY, // TWI state machine ready for use
    TWI_STATE_MASTER_RECEIVER, // TWI state machine is master receiver
    TWI_STATE_MASTER_TRANSMITTER, // TWI state machine is master transmitter
    TWI_STATE_SLAVE_RECEIVER, // TWI state machine is slave receiver
    TWI_STATE_SLAVE_TRANSMITTER  // TWI state machine is slave transmitter
} TWI_STATE_t;

static volatile TWI_STATE_t twi1_MastSlav_RxTx_state;

typedef enum TWI_ACK_enum {
    TWI_ACK,
    TWI_NACK
} TWI_ACK_t;

static volatile TWI1_PROTOCALL_t twi1_protocall; 

typedef enum TWI_ERROR_enum {
    TWI_ERROR_ILLEGAL = TW_BUS_ERROR, // illegal start or stop condition
    TWI_ERROR_MT_SLAVE_ADDR_NACK = TW_MT_SLA_NACK, // Master Transmiter SLA+W transmitted, NACK received 
    TWI_ERROR_MT_DATA_NACK = TW_MT_DATA_NACK, // Master Transmiter data transmitted, NACK received
    TWI_ERROR_ARBITRATION_LOST = TW_MT_ARB_LOST, // Master Transmiter arbitration lost in SLA+W or data
    TWI_ERROR_MS_SLAVE_ADDR_NACK = TW_MR_SLA_NACK, // Master Receiver SLA+W transmitted, NACK received 
    TWI_ERROR_MS_DATA_NACK = TW_MR_DATA_NACK, // Master Receiver data received, NACK returned ()
    // TW_MR_ARB_LOST is done with TW_MT_ARB_LOST
    TWI_ERROR_NONE = 0xFF // No errors
} TWI_ERROR_t;

static volatile TWI_ERROR_t twi1_error;

// used to initalize the slave Transmit functions in case they are not used.
void twi1_transmit_default(void)
{
    // In a real callback, the data to send needs to be copied from a local buffer.
    // uint8_t return_code = twi1_fillSlaveTxBuffer(localBuffer, localBufferLength);
    return;
}

typedef void (*PointerToTransmit)(void);

// used to initalize the slave Receive functions in case they are not used.
void twi1_receive_default(uint8_t *data, uint8_t length)
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

typedef void (*PointerToReceive)(uint8_t*, uint8_t);

static PointerToTransmit twi1_onSlaveTx = twi1_transmit_default;
static PointerToReceive twi1_onSlaveRx = twi1_receive_default;

static uint8_t twi1_masterBuffer[TWI1_BUFFER_LENGTH];
static volatile uint8_t twi1_masterBufferIndex;
static volatile uint8_t twi1_masterBufferLength;

static uint8_t twi1_slaveTxBuffer[TWI1_BUFFER_LENGTH];
static volatile uint8_t twi1_slaveTxBufferIndex;
static volatile uint8_t twi1_slaveTxBufferLength;

// enable interleaving buffer for R-Pi Zero in header
static uint8_t twi1_slaveRxBufferA[TWI1_BUFFER_LENGTH];
#ifdef TWI1_SLAVE_RX_BUFFER_INTERLEAVING
static uint8_t twi1_slaveRxBufferB[TWI1_BUFFER_LENGTH];
#endif

static uint8_t *twi1_slaveRxBuffer;
static volatile uint8_t twi1_slaveRxBufferIndex;

// STOP condition
void twi1_stop_condition(void)
{
    TWCR1 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWSTO);

    // An ISR event does not happen for a stop condition
    while(TWCR1 & (1<<TWSTO))
    {
        continue;
    }

    twi1_MastSlav_RxTx_state = TWI_STATE_READY;
}

// ready the bus but do not STOP it
void twi1_ready_bus(void)
{
    TWCR1 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA) | (1<<TWINT);
    twi1_MastSlav_RxTx_state = TWI_STATE_READY;
}

// ACK or NACK
void twi1_acknowledge(TWI_ACK_t ack)
{
    if(ack == TWI_ACK)
    {
        TWCR1 = (1<<TWEN) | (1<<TWIE) | (1<<TWINT) | (1<<TWEA);
    }
    else
    {
        TWCR1 = (1<<TWEN) | (1<<TWIE) | (1<<TWINT);
    }
}

ISR(TWI1_vect)
{
    switch(TWSR1 & TW_STATUS_MASK) // TW_STATUS can be used for part with one TWI
    {
        // Illegal start or stop condition
        case TW_BUS_ERROR:
            twi1_error = TWI_ERROR_ILLEGAL;
            twi1_stop_condition();
            break;

        // Master start condition transmitted
        case TW_START:
        case TW_REP_START:
            TWDR1 = twi1_slave_read_write;
            twi1_acknowledge(TWI_ACK);
            break;

        // Master Transmitter SLA+W transmitted, ACK received
        case TW_MT_SLA_ACK:
        case TW_MT_DATA_ACK:

            if(twi1_masterBufferIndex < twi1_masterBufferLength)
            {
                TWDR1 = twi1_masterBuffer[twi1_masterBufferIndex++];
                twi1_acknowledge(TWI_ACK);
            }
            else
            {
                if (twi1_protocall & TWI1_PROTOCALL_STOP)
                    twi1_stop_condition();
                else 
                {
                    // Generate the START but set flag for the next transaction.
                    twi1_protocall |= TWI1_PROTOCALL_REPEATEDSTART;
                    TWCR1 = (1<<TWINT) | (1<<TWSTA)| (1<<TWEN) ;
                    twi1_MastSlav_RxTx_state = TWI_STATE_READY;
                }
            }
            break;

        // Master Transmitter SLA+W transmitted, NACK received
        case TW_MT_SLA_NACK:
            twi1_error = TWI_ERROR_MT_SLAVE_ADDR_NACK;
            twi1_stop_condition();
            break;

        // Master Transmitter data transmitted, NACK received
        case TW_MT_DATA_NACK:
            twi1_error = TWI_ERROR_MT_DATA_NACK;
            twi1_stop_condition();
            break;

        // Master Transmitter/Reciver arbitration lost in SLA+W or data
        case TW_MT_ARB_LOST: // same as TW_MR_ARB_LOST
            twi1_error = TWI_ERROR_ARBITRATION_LOST;
            twi1_ready_bus();
            break;

        // Master Reciver Slave SLA+R transmitted, NACK received
        case TW_MR_SLA_NACK:
            twi1_error = TWI_ERROR_MS_SLAVE_ADDR_NACK;
            twi1_stop_condition();
            break;

        // Master Reciver data received, ACK returned
        case TW_MR_DATA_ACK:
            twi1_masterBuffer[twi1_masterBufferIndex++] = TWDR1;
        case TW_MR_SLA_ACK:  // SLA+R transmitted, ACK received
            if(twi1_masterBufferIndex < twi1_masterBufferLength)
            {
                twi1_acknowledge(TWI_ACK);
            }
            else
            {
                twi1_acknowledge(TWI_NACK);
            }
            break;
        
        // Master Reciver data received, NACK returned
        case TW_MR_DATA_NACK:
            twi1_masterBuffer[twi1_masterBufferIndex++] = TWDR1;
            if (twi1_masterBufferIndex < twi1_masterBufferLength) // master returns nack to slave
                twi1_error = TWI_ERROR_MS_DATA_NACK; // but master has done the nack to soon 
            if (twi1_protocall & TWI1_PROTOCALL_STOP)
                twi1_stop_condition();
            else 
            {
                // Generate the START but set flag for the next transaction.
                twi1_protocall |= TWI1_PROTOCALL_REPEATEDSTART;
                TWCR1 = (1<<TWINT) | (1<<TWSTA)| (1<<TWEN) ;
                twi1_MastSlav_RxTx_state = TWI_STATE_READY;
            }    
            break;

        // Slave SLA+W received, ACK returned
        case TW_SR_SLA_ACK:
        case TW_SR_GCALL_ACK:
        case TW_SR_ARB_LOST_SLA_ACK: 
        case TW_SR_ARB_LOST_GCALL_ACK:
            twi1_MastSlav_RxTx_state = TWI_STATE_SLAVE_RECEIVER;
            twi1_slaveRxBufferIndex = 0;
            twi1_acknowledge(TWI_ACK);
            break;

        // Slave data received, ACK returned
        case TW_SR_DATA_ACK: 
        case TW_SR_GCALL_DATA_ACK: 
            if(twi1_slaveRxBufferIndex < TWI1_BUFFER_LENGTH)
            {
                twi1_slaveRxBuffer[twi1_slaveRxBufferIndex++] = TWDR1;
                twi1_acknowledge(TWI_ACK);
            }
            else
            {
                twi1_acknowledge(TWI_NACK);
            }
            break;

        // Slave Data received, return NACK
        case TW_SR_DATA_NACK:       
        case TW_SR_GCALL_DATA_NACK: 
            twi1_acknowledge(TWI_NACK);
            break;

        // Slave stop or repeated start condition received while selected
        case TW_SR_STOP:
            twi1_ready_bus();
            if(twi1_slaveRxBufferIndex < TWI1_BUFFER_LENGTH)
            {
                twi1_slaveRxBuffer[twi1_slaveRxBufferIndex] = '\0';
            }
            twi1_onSlaveRx(twi1_slaveRxBuffer, twi1_slaveRxBufferIndex);
            #ifdef TWI1_SLAVE_RX_BUFFER_INTERLEAVING
            if (twi1_slaveRxBuffer == twi1_slaveRxBufferA) 
            {
                twi1_slaveRxBuffer = twi1_slaveRxBufferB;
            }
            else
            {
                twi1_slaveRxBuffer = twi1_slaveRxBufferA;
            }
            #endif
            twi1_slaveRxBufferIndex = 0;
            break;

        // Slave SLA+R received, ACK returned
        case TW_ST_SLA_ACK: 
        case TW_ST_ARB_LOST_SLA_ACK:
            twi1_MastSlav_RxTx_state = TWI_STATE_SLAVE_TRANSMITTER;
            twi1_slaveTxBufferIndex = 0;
            twi1_slaveTxBufferLength = 0; 
            twi1_onSlaveTx(); // use twi1_fillSlaveTxBuffer(bytes, length) in callback
            if(0 == twi1_slaveTxBufferLength) // default callback does not set this
            {
                twi1_slaveTxBufferLength = 1;
                twi1_slaveTxBuffer[0] = 0x00;
            }
        case TW_ST_DATA_ACK:
            TWDR1 = twi1_slaveTxBuffer[twi1_slaveTxBufferIndex++];
            if(twi1_slaveTxBufferIndex < twi1_slaveTxBufferLength)
            {
                twi1_acknowledge(TWI_ACK);
            }
            else
            {
                twi1_acknowledge(TWI_NACK);
            }
            break;

        // Slave data transmitted, ACK or NACK received
        case TW_ST_LAST_DATA:
        case TW_ST_DATA_NACK:
            twi1_acknowledge(TWI_ACK);
            twi1_MastSlav_RxTx_state = TWI_STATE_READY;
            break;

        // Some ISR events get ignored
        case TW_NO_INFO: 
            break;
    }
}

/*************** PUBLIC ***********************************/

// Initialize TWI1 module (bitrate, pull-up)
// if bitrate is 0 then disable twi, a normal bitrate is 100000UL, 
void twi1_init(uint32_t bitrate, TWI1_PINS_t pull_up)
{
    if (bitrate == 0)
    {
        // disable twi module, acks, and twi interrupt
        TWCR1 &= ~((1<<TWEN) | (1<<TWIE) | (1<<TWEA));

        // deactivate internal pullups for twi.
        ioWrite(MCU_IO_SCL1, LOGIC_LEVEL_LOW); // PORTE &= ~(1 << PORTE6) disable the pull-up
        ioWrite(MCU_IO_SDA1, LOGIC_LEVEL_LOW); // PORTE &= ~(1 << PORTE5)
    }
    else
    {
        // start with interleaving buffer A (B is an optional buffer if it is enabled)
        twi1_slaveRxBuffer = twi1_slaveRxBufferA;

        // initialize state machine
        twi1_MastSlav_RxTx_state = TWI_STATE_READY;
        twi1_protocall = TWI1_PROTOCALL_STOP & ~TWI1_PROTOCALL_REPEATEDSTART;

        ioDir(MCU_IO_SCL1, DIRECTION_INPUT); // DDRE &= ~(1 << DDE6)
        ioDir(MCU_IO_SDA1, DIRECTION_INPUT); // DDRE &= ~(1 << DDE5)

        // weak pullup pull-up.
        if (pull_up == TWI1_PINS_PULLUP)
        {
            ioWrite(MCU_IO_SCL1, LOGIC_LEVEL_HIGH); // PORTE |= (1 << PORTE6)
            ioWrite(MCU_IO_SDA1, LOGIC_LEVEL_HIGH); // PORTE |= (1 << PORTE5)
        }

        // initialize TWPS[0:1]=0 for a prescaler = 1
        TWSR1 &= ~((1<<TWPS0));
        TWSR1 &= ~((1<<TWPS1));

        // bitrate = (F_CPU)/(16+(2*TWBR1*prescaler))
        // TWBR1 = ((F_CPU) - bitrate*16)/(bitrate*2*prescaler)
        // TWBR1 = ((F_CPU/bitrate) - 16)/(2*prescaler)
        TWBR1 = ((F_CPU / bitrate) - 16) / 2; //I2C default is 100000L
        // At 16MHz TWBR1 is 72 which is saved in TWDR1 and 
        // the bitrate is perfect (F_CPU)/(16+(2*TWBR1*1)) = 100000

        // enable twi module, acks, and twi interrupt
        TWCR1 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA);
    }
}

// TWI Asynchronous Write Transaction.
// 0 .. Transaction started, check status for success
// 1 .. to much data, it did not fit in buffer
// 2 .. TWI state machine not ready for use
TWI1_WRT_t twi1_masterAsyncWrite(uint8_t slave_address, uint8_t *write_data, uint8_t bytes_to_write, TWI1_PROTOCALL_t send_stop)
{
    uint8_t i;

    if(bytes_to_write > TWI1_BUFFER_LENGTH)
    {
        return TWI1_WRT_TO_MUCH_DATA;
    }
    else
    {    
        if(twi1_MastSlav_RxTx_state != TWI_STATE_READY)
        {
            return TWI1_WRT_NOT_READY;
        }
        else // do TWI1_WRT_TRANSACTION_STARTED
        {
            twi1_MastSlav_RxTx_state = TWI_STATE_MASTER_TRANSMITTER;
            if (send_stop == TWI1_PROTOCALL_STOP)
            {
                twi1_protocall |= TWI1_PROTOCALL_STOP;
            }
            else
            {
                twi1_protocall &= ~TWI1_PROTOCALL_STOP;
            }
            
            twi1_error = TWI_ERROR_NONE;
            twi1_masterBufferIndex = 0;
            twi1_masterBufferLength = bytes_to_write;
            for(i = 0; i < bytes_to_write; ++i)
            {
                twi1_masterBuffer[i] = write_data[i];
            }
        
            // build SLA+W, slave device address + write bit
            twi1_slave_read_write = slave_address << 1;
            twi1_slave_read_write += TW_WRITE;
        
            // Check if ISR has done the I2C START
            if (twi1_protocall & TWI1_PROTOCALL_REPEATEDSTART) 
                {
                uint8_t local_twi_protocall = twi1_protocall;
                local_twi_protocall &= ~TWI1_PROTOCALL_REPEATEDSTART;
                twi1_protocall = local_twi_protocall;
                do 
                {
                    TWDR1 = twi1_slave_read_write;
                } while(TWCR1 & (1<<TWWC));

                // enable INTs, skip START
                TWCR1 = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE);
            }
            else
            {
                // enable INTs and START
                TWCR1 = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE) | (1<<TWSTA);
            }
            return TWI1_WRT_TRANSACTION_STARTED;
        }
    }
}

// TWI master write transaction status.
TWI1_WRT_STAT_t twi1_masterAsyncWrite_status(void)
{
    if (TWI_STATE_MASTER_TRANSMITTER == twi1_MastSlav_RxTx_state)
        return TWI1_WRT_STAT_BUSY;
    else if (TWI_ERROR_NONE == twi1_error)
        return TWI1_WRT_STAT_SUCCESS;
    else if (TWI_ERROR_MT_SLAVE_ADDR_NACK == twi1_error) 
        return TWI1_WRT_STAT_ADDR_NACK;
    else if (TWI_ERROR_MT_DATA_NACK == twi1_error) 
        return TWI1_WRT_STAT_DATA_NACK;
    else if (TWI_ERROR_ILLEGAL == twi1_error) 
        return TWI1_WRT_STAT_ILLEGAL;
    else 
        return 5; // can not happen
}


// TWI write with a state machine so the wait can be done elsewhere
// loop until loop_state == TWI1_LOOP_STATE_DONE, then return value is 
// 0 .. success
// 1 .. length to long for buffer
// 2 .. address send, NACK received
// 3 .. data send, NACK received
// 4 .. illegal start or stop condition
uint8_t twi1_masterWrite(uint8_t slave_address, uint8_t* write_data, uint8_t bytes_to_write, TWI1_PROTOCALL_t send_stop, TWI1_LOOP_STATE_t *loop_state)
{
    TWI1_WRT_t twi_state_machine = TWI1_WRT_TRANSACTION_STARTED;
    TWI1_WRT_STAT_t status = TWI1_WRT_STAT_SUCCESS;
    switch (*loop_state)
    {
        case TWI1_LOOP_STATE_RAW:
        case TWI1_LOOP_STATE_DONE:
            break; // report success, there was nothing to do
        case TWI1_LOOP_STATE_ASYNC_WRT:
            twi_state_machine = twi1_masterAsyncWrite(slave_address, write_data, bytes_to_write, send_stop);
            if (twi_state_machine == TWI1_WRT_TO_MUCH_DATA) 
            {
                *loop_state = TWI1_LOOP_STATE_DONE;
                status = TWI1_WRT_STAT_BUSY; // report TWI1_WRT_STAT_BUSY (1) when TWI1_WRT_TO_MUCH_DATA occures
                break; // data did not fit in the buffer, request ignored
            }
            else if (twi_state_machine == TWI1_WRT_NOT_READY)
            {
                break; // but the twi state machine is in use.
            }
            else 
            {
                *loop_state = TWI1_LOOP_STATE_STATUS_WRT;
                break; // the twi state machine was given data and made ready.
            }
        case TWI1_LOOP_STATE_STATUS_WRT:
            status = twi1_masterAsyncWrite_status();
            if (status == TWI1_WRT_STAT_BUSY)
            {
                break; // the twi state machine has the data and we are waiting for it to finish.
            }
            else
            {
                *loop_state = TWI1_LOOP_STATE_DONE;
                break; // all done
            }
        case TWI1_LOOP_STATE_INIT:
        case TWI1_LOOP_STATE_ASYNC_RD:
        case TWI1_LOOP_STATE_STATUS_RD:
            {
                *loop_state = TWI1_LOOP_STATE_DONE;
                break; // wrong state was set befor running
            }
    }
    return status; // note that TWI1_WRT_STAT_BUSY (1) is reported when TWI1_WRT_TO_MUCH_DATA occures
}

// TWI write busy-wait transaction, do not use with multi-master.
// 0 .. success
// 1 .. length to long for buffer
// 2 .. address send, NACK received
// 3 .. data send, NACK received
// 4 .. illegal start or stop condition
uint8_t twi1_masterBlockingWrite(uint8_t slave_address, uint8_t* write_data, uint8_t bytes_to_write, TWI1_PROTOCALL_t send_stop)
{
    uint8_t twi_wrt_code = 0;
    TWI1_LOOP_STATE_t loop_state = TWI1_LOOP_STATE_ASYNC_WRT; // loop state is in this blocking function rather than in the main loop
    while (loop_state != TWI1_LOOP_STATE_DONE)
    {
        twi_wrt_code = twi1_masterWrite(slave_address, write_data, bytes_to_write, send_stop, &loop_state);
    }
    return twi_wrt_code;
}

// TWI Asynchronous Read Transaction.
// 0 .. data fit in buffer, check twi1_masterAsyncRead_bytesRead for when it is done
// 1 .. data will not fit in the buffer, request ignored
// 2 .. TWI state machine not ready for use
TWI1_RD_t twi1_masterAsyncRead(uint8_t slave_address, uint8_t bytes_to_read, TWI1_PROTOCALL_t send_stop)
{
    if(bytes_to_read > TWI1_BUFFER_LENGTH)
    {
        return TWI1_RD_TO_MUCH_DATA;
    }
    else
    {
        if (TWI_STATE_READY != twi1_MastSlav_RxTx_state)
        {
            return TWI1_RD_NOT_READY;
        }
        else
        {
            twi1_MastSlav_RxTx_state = TWI_STATE_MASTER_RECEIVER;
            if (send_stop == TWI1_PROTOCALL_STOP)
            {
                twi1_protocall |= TWI1_PROTOCALL_STOP;
            }
            else
            {
                twi1_protocall &= ~TWI1_PROTOCALL_STOP;
            }
            twi1_error = TWI_ERROR_NONE;

            twi1_masterBufferIndex = 0;

            // set NACK when the _next_ to last byte received. 
            twi1_masterBufferLength = bytes_to_read-1; 

            // build SLA+R, slave device address + r bit
            twi1_slave_read_write = slave_address << 1;
            twi1_slave_read_write += TW_READ;


            // Check if ISR has done the I2C START
            if (twi1_protocall & TWI1_PROTOCALL_REPEATEDSTART)
            {
                uint8_t local_twi_protocall = twi1_protocall;
                local_twi_protocall &= ~TWI1_PROTOCALL_REPEATEDSTART;
                twi1_protocall = local_twi_protocall;
                do 
                {
                    TWDR1 = twi1_slave_read_write;
                } while(TWCR1 & (1<<TWWC));
                TWCR1 = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE);	// enable INTs, but not START
            }
            else
            {
                // send start condition
                TWCR1 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWSTA);
            }
            return TWI1_RD_TRANSACTION_STARTED;
        }
    }
}

// TWI master Asynchronous Read Transaction status.
TWI1_RD_STAT_t twi1_masterAsyncRead_status(void)
{
    if (TWI_STATE_MASTER_RECEIVER == twi1_MastSlav_RxTx_state)
        return TWI1_RD_STAT_BUSY;
    else if (TWI_ERROR_NONE == twi1_error)
        return TWI1_RD_STAT_SUCCESS;
    else if (TWI_ERROR_MS_SLAVE_ADDR_NACK == twi1_error) 
        return TWI1_RD_STAT_ADDR_NACK;
    else if (TWI_ERROR_MS_DATA_NACK == twi1_error) 
        return TWI1_RD_STAT_DATA_NACK;
    else if (TWI_ERROR_ILLEGAL == twi1_error) 
        return TWI1_RD_STAT_ILLEGAL;
    else 
        return 5; // can not happen
}

// TWI master Asynchronous Read Transaction get bytes.
// Output   0 .. twi busy, read operation not complete or has error (check status)
//      1..32 .. number of bytes read
uint8_t twi1_masterAsyncRead_getBytes(uint8_t *read_data)
{
    if ( (twi1_MastSlav_RxTx_state == TWI_STATE_MASTER_RECEIVER) && (TWI_ERROR_NONE != twi1_error) )
    {
        return 0;
    }

    uint8_t bytes_read = twi1_masterBufferLength+1;
    if (twi1_masterBufferIndex < bytes_read )
    {
        bytes_read = twi1_masterBufferIndex;
    }

    uint8_t i;
    for(i = 0; i < bytes_read; ++i)
    {
        read_data[i] = twi1_masterBuffer[i];
    }
	
    return bytes_read;
}

// TWI read with a state machine so the wait can be done elsewhere
// loop until loop_state == TWI1_LOOP_STATE_DONE, then return value is 
// 0 returns when somthing went wrong, was it from TWI1_RD_TO_MUCH_DATA, or check twi1_masterAsyncRead_status
// 1..32 returns the number of bytes
uint8_t twi1_masterRead(uint8_t slave_address, uint8_t* read_data, uint8_t bytes_to_read, TWI1_PROTOCALL_t send_stop, TWI1_LOOP_STATE_t *loop_state)
{
    TWI1_RD_t twi_state_machine = TWI1_RD_TRANSACTION_STARTED;
    TWI1_RD_STAT_t status = TWI1_RD_STAT_BUSY;
    uint8_t bytes_read = 0;
    switch (*loop_state)
    {
        case TWI1_LOOP_STATE_RAW:
        case TWI1_LOOP_STATE_DONE:
            break; // there was nothing to do
        case TWI1_LOOP_STATE_ASYNC_RD:
            twi_state_machine = twi1_masterAsyncRead(slave_address, bytes_to_read, send_stop);
            if (twi_state_machine == TWI1_RD_TO_MUCH_DATA) 
            {
                *loop_state = TWI1_LOOP_STATE_DONE; // done, 
                break; // read request ignored, data did not fit in the buffer.
            }
            else if (twi_state_machine == TWI1_RD_NOT_READY)
            {
                break; // but the twi state machine is in use.
            }
            else 
            {
                *loop_state = TWI1_LOOP_STATE_STATUS_RD;
                break; // machine was told to read data and made ready.
            }
        case TWI1_LOOP_STATE_STATUS_RD:
            status = twi1_masterAsyncRead_status();
            if (status == TWI1_RD_STAT_BUSY)
            {
                break; // state machine is getting the data and we are waiting for it to finish.
            }
            else if ( (status == TWI1_RD_STAT_ADDR_NACK) || (status == TWI1_RD_STAT_DATA_NACK) || (status == TWI1_RD_STAT_ILLEGAL) ) // read faild 
            {
                *loop_state = TWI1_LOOP_STATE_DONE; // done,
                break; // read failed, the status command can be used again to get the error
            }
            else
            {
                bytes_read = twi1_masterAsyncRead_getBytes(read_data);
                *loop_state = TWI1_LOOP_STATE_DONE;
                break; // all done
            }
        case TWI1_LOOP_STATE_INIT:
        case TWI1_LOOP_STATE_ASYNC_WRT:
        case TWI1_LOOP_STATE_STATUS_WRT:
            {
                *loop_state = TWI1_LOOP_STATE_DONE;
                break; // wrong state was set befor running
            }
    }
    return bytes_read;
}



// TWI read busy-wait transaction, do not use with multi-master.
// 0 returns if requeste for data will not fit in buffer
// 1..32 returns the number of bytes
uint8_t twi1_masterBlockingRead(uint8_t slave_address, uint8_t* read_data, uint8_t bytes_to_read, TWI1_PROTOCALL_t send_stop)
{
    uint8_t bytes_read = 0;
    TWI1_LOOP_STATE_t loop_state = TWI1_LOOP_STATE_ASYNC_RD; // loop state is in this blocking function rather than in the main loop
    while (loop_state != TWI1_LOOP_STATE_DONE)
    {
        bytes_read = twi1_masterRead(slave_address, read_data, bytes_to_read, send_stop, &loop_state);
    }
    return bytes_read;
}

// TWI Write and then Read with a state machine so the wait can be done elsewhere
// befor calling define a variable for the stat machine to use 
// static TWI1_LOOP_STATE_t twi0_loop_state = TWI1_LOOP_STATE_DONE;
// each time the loop_state machine is used it needs started with
// twi0_loop_state = TWI1_LOOP_STATE_ASYNC_WRT;
// when operations are finished twi0_loop_state == TWI1_LOOP_STATE_DONE 
// and the return value should have the bytes_read in lower 5 bits (0..32)
// error codes are moved to the upper 3 bits (e.g., twi_wrt_code<<5 or twi1_masterAsyncRead_status()<<5)
uint8_t twi1_masterWriteRead(uint8_t slave_address, uint8_t* write_data, uint8_t bytes_to_write, uint8_t* read_data, uint8_t bytes_to_read, TWI1_LOOP_STATE_t *loop_state)
{
    if ( (*loop_state == TWI1_LOOP_STATE_ASYNC_WRT) || (*loop_state == TWI1_LOOP_STATE_STATUS_WRT) )
    {
        uint8_t twi_wrt_code = twi1_masterWrite(slave_address, write_data, bytes_to_write, TWI1_PROTOCALL_REPEATEDSTART, loop_state);
        if ( (*loop_state == TWI1_LOOP_STATE_DONE) && (twi_wrt_code == 0) )
        {
            *loop_state = TWI1_LOOP_STATE_ASYNC_RD;
        }
        else
        {
            // twi_wrt_code may have an error in which case loop_state is set as TWI0_LOOP_STATE_DONE
            // or write is not done and we can check again after a spin through the outside loop
            return twi_wrt_code<<5; // if twi_wrt_code has an error use twi0_masterAsyncWrite_status to see it
        }
    }

    uint8_t bytes_read = twi1_masterRead(slave_address, read_data, bytes_to_read, TWI1_PROTOCALL_STOP, loop_state);
    if (*loop_state == TWI1_LOOP_STATE_DONE)
    {
        if (bytes_read)
        {
            return bytes_read;
        }
        else
        {
            return twi1_masterAsyncRead_status()<<5;
        }
    }
    else
    {
        return 0; // not done
    }
    
}



// set valid slave address (0x8..0x77) 
// return address if set
uint8_t twi1_slaveAddress(uint8_t slave)
{
    if( (slave>=0x8) && (slave<=0x77))
    {
        // TWAR1 is Slave Address Register TWA[6..0] in bits 7..1 TWGCE in bit 0
        //       TWGCE bit is for General Call Recognition
        TWAR1 = slave << 1; 
        return slave;
    }
    else
    {
        TWAR1 = 0; 
        twi1_onSlaveTx = twi1_transmit_default;
        twi1_onSlaveRx = twi1_receive_default;
        return 0;
    }
}

// fill twi1_slaveTxBuffer using callback returns
// 0: OK
// 1: bytes_to_send is to much for buffer, so request ignored
// 2: TWI state machine is not in slave mode, so request ignored
uint8_t twi1_fillSlaveTxBuffer(const uint8_t* slave_data, uint8_t bytes_to_send)
{
    if(TWI1_BUFFER_LENGTH < bytes_to_send)
    {
        return 1;
    }
  
    if(TWI_STATE_SLAVE_TRANSMITTER != twi1_MastSlav_RxTx_state)
    {
        return 2;
    }
  
    twi1_slaveTxBufferLength = bytes_to_send;
    for(uint8_t i = 0; i < bytes_to_send; ++i)
    {
        twi1_slaveTxBuffer[i] = slave_data[i];
    }
  
    return 0;
}

// record callback to use durring a slave read operation 
// a NULL pointer will use the default callback
void twi1_registerSlaveRxCallback( void (*function)(uint8_t*, uint8_t) )
{
    if (function == ((void *)0) )
    {
        twi1_onSlaveRx = twi1_receive_default;
    }
    else
    {
        twi1_onSlaveRx = function;
    }
}

// record callback to use before a slave write operation
// a NULL pointer will use the default callback
void twi1_registerSlaveTxCallback( void (*function)(void) )
{
    if (function == ((void *)0) )
    {
        twi1_onSlaveTx = twi1_transmit_default;
    }
    else
    {
        twi1_onSlaveTx = function;
    }
}
