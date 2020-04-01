/*
Manager functions are done over the i2c interface between application controller and manager
Copyright (c) 2017 Ronald S. Sutherland

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

#include <stdio.h>
#include "twi0_bsd.h"
#include "rpu_mgr.h"

// 1 .. length to long for buffer 
// 2 .. address send, NACK received 
// 3 .. data send, NACK received 
// 4 .. other twi error (e.g., lost bus arbitration, bus error) 
// 5 .. read does not match length
// 6 .. bad command
// 7 .. prevent sending bad data
uint8_t twi_errorCode;

// command 0 is used to read the address from manager
#define ADDRESS_CMD {0x00,0x00}
#define ADDRESS_CMD_SIZE 2

// command 4 reads the shutdown detected status
#define SHUTDOWN_DETECT_CMD {0x04,0xFF}
#define SHUTDOWN_DETECT_CMD_SIZE 2

// command 5 has the manager pull down the shutdown 
// pin for a time so the host can see it and hault
#define SHUTDOWN_CMD {0x05,0x01}
#define SHUTDOWN_CMD_SIZE 2

// command 6 reads the status bits
#define STATUS_READ_CMD {0x06,0xFF}
#define STATUS_READ_CMD_SIZE 2

// commands 21, 22 have the manger set and report an int.
// The morning and evening threshold 
#define INT_CMD {0x15,0x00,0x00}
#define INT_CMD_SIZE 3

// commands have the manger set and report a byte.
// e.g., 23 is used to accesses Day-Night state (low and high nibble). 
#define UINT8_CMD {0x17,0x00}
#define UINT8_CMD_SIZE 2

// commands 32 will have the manger do an 
// analogRead and pass that to the application
#define ANALOG_RD_CMD {0x20,0x00,0x00}
#define ANALOG_RD_CMD_SIZE 3

// commands 52, 53 and 54  have the manger set and report an
// unsigned long. The evening|morning|millis_now debouce 
// out of range values are ignored
#define ULONGINT_CMD {0x34,0x00,0x00,0x00,0x00}
#define ULONGINT_CMD_SIZE 5

#define RPU_BUS_MSTR_CMD_SZ 2
#define I2C_ADDR_OF_BUS_MGR 0x29

// cycle the twi state machine on both the master and slave(s)
void i2c_ping(void)
{ 
    // ping I2C for an RPU bus manager 
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR;
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

// The manager can pull down the shutdown pin (just like the manual switch) 
// that the Raspberry Pi monitors for halting its operating system.
uint8_t i2c_set_Rpu_shutdown(void)
{ 
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR;
    
    // Send the host shutdown command to manager, this should cause 
    // the manager to pull down its pin used to signal host to shutdown
    uint8_t txBuffer[SHUTDOWN_CMD_SIZE] = SHUTDOWN_CMD; 
    uint8_t length = SHUTDOWN_CMD_SIZE;
    twi_errorCode = twi0_masterBlockingWrite(i2c_address, txBuffer, length, TWI0_PROTOCALL_REPEATEDSTART); 
    if (twi_errorCode)
    {
        return 0;
    }
    
    // above writes data to manager, this reads data from manager which sends back the same length that was sent
    uint8_t rxBuffer[SHUTDOWN_CMD_SIZE];
    uint8_t bytes_read = twi0_masterBlockingRead(i2c_address, rxBuffer, length, TWI0_PROTOCALL_STOP);
    if ( bytes_read != length )
    {
        twi_errorCode = 5;
        return 0;
    }
    else
    {
        if ( rxBuffer[1] == txBuffer[1] )
        {
            return 1; // all seems good
        }
        else 
        {
            return 0;
        }
    }
}

// The manager keeps track of when the shutdown pin (manual switch or 
// i2c/SMBus command) is pulled low. Reading will clear the record.
uint8_t i2c_detect_Rpu_shutdown(void)
{ 
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR;

    // Send the host shutdown detect command to manager, the manager will return the 
    // shutdown_detected value and then clear it.
    // note: the host can request shutdown via SMBus and this will retain clues for the application.
    uint8_t txBuffer[SHUTDOWN_DETECT_CMD_SIZE] = SHUTDOWN_DETECT_CMD; //detect host shutdown comand 0x04, data place holder 0xFF;
    uint8_t length = SHUTDOWN_DETECT_CMD_SIZE;
    twi_errorCode = twi0_masterBlockingWrite(i2c_address, txBuffer, length, TWI0_PROTOCALL_REPEATEDSTART); 
    if (twi_errorCode)
    {
        return 0; // failed
    }
    
    // above writes data to slave, this reads data from slave
    uint8_t rxBuffer[SHUTDOWN_DETECT_CMD_SIZE];
    uint8_t bytes_read = twi0_masterBlockingRead(i2c_address, rxBuffer, length, TWI0_PROTOCALL_STOP);
    if ( bytes_read != length )
    {
        twi_errorCode = 5;
        return 0;
    }
    else
    {
        return rxBuffer[1];
    }
}

// The manager has the mulitdorp serial address. When I read it 
// over I2C the manage will boradcast a command on its out of band
// channel that places all devices in normal mode (e.g., not p2p or bootload) 
char i2c_get_Rpu_address(void)
{ 
    i2c_ping();
    if ( twi_errorCode ) return 0;
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR;    
    uint8_t txBuffer[ADDRESS_CMD_SIZE] = ADDRESS_CMD;
    uint8_t length = ADDRESS_CMD_SIZE;
    twi_errorCode = twi0_masterBlockingWrite(i2c_address, txBuffer, length, TWI0_PROTOCALL_REPEATEDSTART); 
    if (twi_errorCode)
    {
        return 0; // failed
    }

    uint8_t rxBuffer[ADDRESS_CMD_SIZE];
    uint8_t bytes_read = twi0_masterBlockingRead(i2c_address, rxBuffer, length, TWI0_PROTOCALL_STOP);
    if ( bytes_read != length )
    {
        twi_errorCode = 5;
        return 0;
    }
    else
    {
        return (char)(rxBuffer[1]);
    }
}

// I2C command 32 takes a channel and returns adc[channel]
// channels are ALT_I | ALT_V | PWR_I | PWR_V
int i2c_get_adc_from_manager(uint8_t channel, TWI0_LOOP_STATE_t *loop_state)
{
    // use the int access cmd
    return i2c_int_access_cmd(0x20, (int)channel, loop_state);
}

// The manager has a status byte that has the following bits. 
// 0 .. DTR readback timeout
// 1 .. twi fail
// 2 .. DTR readback not match
// 3 .. host lockout
// 4 .. alternate power enable (ALT_EN) 
// 5 .. SBC power enable (PIPWR_EN) 
// 6 .. daynight fail
uint8_t i2c_read_status(void)
{ 
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR;

    uint8_t txBuffer[STATUS_READ_CMD_SIZE] = STATUS_READ_CMD; //detect host shutdown comand 0x04, data place holder 0xFF;
    uint8_t length = STATUS_READ_CMD_SIZE;
    twi_errorCode = twi0_masterBlockingWrite(i2c_address, txBuffer, length, TWI0_PROTOCALL_REPEATEDSTART); 
    if (twi_errorCode)
    {
        return 0x02; // set twi fail bit, even though it is not from manager
    }
    
    // above writes data to slave, this reads data from slave
    uint8_t rxBuffer[STATUS_READ_CMD_SIZE];
    uint8_t bytes_read = twi0_masterBlockingRead(i2c_address, rxBuffer, length, TWI0_PROTOCALL_STOP);
    if ( bytes_read != length )
    {
        twi_errorCode = 5;
        return 0x02;
    }
    else
    {
        return rxBuffer[1]; // return manager status
    }
}

// management commands to access managers uint8 prameters e.g., 
// 23 .. DAYNIGHT_STATE with state in low nibble and work notice in high nibble. 
//       state values range from: 0..7 and 
//       bit 7=night_work, 6=day_work, 5 is set to see 6 and 7, 4 is set to clear 6 and 7.
uint8_t i2c_uint8_access_cmd(uint8_t command, uint8_t update_with)
{ 
    if ( (command != 23) ) 
    {
        twi_errorCode = 6;
        return 0;
    }
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR;
    uint8_t txBuffer[UINT8_CMD_SIZE] = UINT8_CMD;
    uint8_t length = UINT8_CMD_SIZE;
    txBuffer[0] = command; // replace the command byte
    txBuffer[1] = update_with;
    twi_errorCode = twi0_masterBlockingWrite(i2c_address, txBuffer, length, TWI0_PROTOCALL_REPEATEDSTART); 
    if (twi_errorCode)
    {
        return 0; // failed
    }
    
    // above writes data to slave, this reads data from slave
    uint8_t rxBuffer[UINT8_CMD_SIZE];
    uint8_t bytes_read = twi0_masterBlockingRead(i2c_address, rxBuffer, length, TWI0_PROTOCALL_STOP);
    if ( bytes_read != length )
    {
        twi_errorCode = 5;
        return 0;
    }
    return rxBuffer[1];
}

// management commands to access managers unsigned long prameters e.g.,
// 20 .. CHARGE_BATTERY_ABSORPTION 
// 52 .. EVENING_DEBOUNCE 
// 53 .. MORNING_DEBOUNCE 
// 54 .. DAYNIGHT_TIMER
unsigned long i2c_ul_access_cmd(uint8_t command, unsigned long update_with)
{
    if ( ((command<52) | (command>54)) & (command != 20) ) 
    {
        twi_errorCode = 6;
        return 0;
    }
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR; //0x29
    uint8_t length = ULONGINT_CMD_SIZE;
    uint8_t txBuffer[ULONGINT_CMD_SIZE] = ULONGINT_CMD;
    txBuffer[0] = command; // replace the command byte
    txBuffer[3] = (uint8_t)((update_with & 0xFF000000UL)>>24);
    txBuffer[3] = (uint8_t)((update_with & 0xFF0000UL)>>16);
    txBuffer[3] = (uint8_t)((update_with & 0xFF00UL)>>8);
    txBuffer[4] = (uint8_t)(update_with & 0xFFUL);
    twi_errorCode = twi0_masterBlockingWrite(i2c_address, txBuffer, length, TWI0_PROTOCALL_REPEATEDSTART); 
    if (twi_errorCode)
    {
        return 0;
    }
    uint8_t rxBuffer[ULONGINT_CMD_SIZE];
    uint8_t bytes_read = twi0_masterBlockingRead(i2c_address, rxBuffer, length, TWI0_PROTOCALL_STOP);
    if ( bytes_read != length )
    {
        twi_errorCode = 5;
        return 0;
    }
    unsigned long value = ((unsigned long)(rxBuffer[1]))<<24;
    value += ((unsigned long)(rxBuffer[2]))<<16;
    value += ((unsigned long)(rxBuffer[3]))<<8;
    value +=  (unsigned long)rxBuffer[4];
    return value;
}

// management commands that take an int to update and return an int e.g. 
// 18 .. CHARGE_BATTERY_START 
// 19 .. CHARGE_BATTERY_STOP 
// 21 .. MORNING_THRESHOLD daynight threshold prameter
// 22 .. EVENING_THRESHOLD daynight threshold prameter
// 32 .. takes a ADC_CH_MGR_enum and returns the 10 bit adc reading (ALT_I | ALT_V | PWR_I | PWR_V)
int i2c_int_access_cmd(uint8_t command, int update_with, TWI0_LOOP_STATE_t *loop_state)
{
    if ( (command != 32) & (((command<18) | (command>22)) | (command==20)) ) 
    {
        twi_errorCode = 6;
        return 0;
    }
    if ( (command == 32) & (update_with >= ADC_CH_MGR_MAX_NOT_A_CH) )
    {
        twi_errorCode = 7;
        return 0;
    }
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR; //0x29
    uint8_t length = INT_CMD_SIZE;
    uint8_t txBuffer[INT_CMD_SIZE] = INT_CMD; 
    txBuffer[0] = command; // replace the command byte
    txBuffer[1] = (uint8_t)((update_with & 0xFF00)>>8);
    txBuffer[2] = (uint8_t)(update_with & 0xFF);
    uint8_t rxBuffer[INT_CMD_SIZE];
    uint8_t bytes_read = twi0_masterWriteRead(i2c_address, txBuffer, length, rxBuffer, length, loop_state);
    /* keep as a known working referance for now
    twi_errorCode = twi0_masterBlockingWrite(i2c_address, txBuffer, length, TWI0_PROTOCALL_REPEATEDSTART); 
    if (twi_errorCode)
    {
        return 0;
    }
    uint8_t bytes_read = twi0_masterBlockingRead(i2c_address, rxBuffer, length, TWI0_PROTOCALL_STOP);
    if ( bytes_read == 0 )
    {
        twi_errorCode = twi0_masterAsyncRead_status();
        return 0;
    }
    *loop_state = TWI0_LOOP_STATE_DONE;
    */
    int value = 0;
    if( (*loop_state == TWI0_LOOP_STATE_DONE) /* && (bytes_read == length) */ )
    {
        // twi0_masterWriteRead error code is in bits 5..7
        if(bytes_read & 0xE0)
        {
            twi_errorCode = bytes_read>>5;
            value = (int) (-twi_errorCode); // use a neg value as the error code
        }
        else
        {
            value = ((int)(rxBuffer[1]))<<8;
            value +=  (int)rxBuffer[2];
        }
    }
    return value;
}
