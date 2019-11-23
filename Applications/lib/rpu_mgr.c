/*
  Manager functions are done over the i2c interface between application controller and manager
  Copyright (c) 2017 Ronald S. Sutherland

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

#include <stdio.h>
#include "twi0.h"
#include "rpu_mgr.h"

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

// command 23 accesses Day-Night state in low nibble. 
// and work needs done in high nibble
#define DAYNIGHT_STATE_CMD {0x17,0x20}
#define DAYNIGHT_STATE_CMD_SIZE 2

// commands 32, 33, 34 and 35 have the manger do an 
// analogRead and pass that to the application
#define ANALOG_RD_CMD {0x20,0xFF,0xFF}
#define ANALOG_RD_CMD_SIZE 3

// commands 52, 53 and 54  have the manger set and report an
// unsigned long. The evening|morning|millis_now debouce 
// out of range values are ignored
#define ULONGINT_CMD {0x34,0x00,0x00,0x00,0x00}
#define ULONGINT_CMD_SIZE 5

#define RPU_BUS_MSTR_CMD_SZ 2
#define I2C_ADDR_OF_BUS_MGR 0x29

// The manager can pull down the shutdown pin (just like the manual switch) 
// that the Raspberry Pi monitors for halting its operating system.
uint8_t i2c_set_Rpu_shutdown(void)
{ 
    uint8_t twi_returnCode;
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR;
    
    // Send the host shutdown command to manager, this should cause 
    // the manager to pull down its pin used to signal host to shutdown
    uint8_t txBuffer[SHUTDOWN_CMD_SIZE] = SHUTDOWN_CMD; 
    uint8_t length = SHUTDOWN_CMD_SIZE;
    uint8_t wait = 1;
    uint8_t sendStop = 0;  //this will cause a I2C repeated Start durring read
    twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop); 
    if (twi_returnCode != 0)
    {
        return 0; // nack failed
    }
    
    // above writes data to manager, this reads data from manager which sends back the same length that was sent
    uint8_t rxBuffer[SHUTDOWN_CMD_SIZE];
    sendStop = 1;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, length, sendStop);
    if ( bytes_read != length )
    {
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
    uint8_t twi_returnCode;
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR;

    // Send the host shutdown detect command to manager, the manager will return the 
    // shutdown_detected value and then clear it.
    // note: the host can request shutdown via SMBus and this will retain clues for the application.
    uint8_t txBuffer[SHUTDOWN_DETECT_CMD_SIZE] = SHUTDOWN_DETECT_CMD; //detect host shutdown comand 0x04, data place holder 0xFF;
    uint8_t length = SHUTDOWN_DETECT_CMD_SIZE;
    uint8_t wait = 1;
    uint8_t sendStop = 0;  //this will cause a I2C repeated Start durring read
    twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop); 
    if (twi_returnCode != 0)
    {
        return 0; // nack failed
    }
    
    // above writes data to slave, this reads data from slave
    uint8_t rxBuffer[SHUTDOWN_DETECT_CMD_SIZE];
    sendStop = 1;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, length, sendStop);
    if ( bytes_read != length )
    {
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
    uint8_t twi_returnCode;

    // ping I2C for an RPU bus manager 
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR;
    uint8_t data = 0;
    uint8_t length = 0;
    uint8_t wait = 1;
    uint8_t sendStop = 1;
    for (uint8_t i =0;1; i++) // try a few times, it is slower starting after power up.
    {
        twi_returnCode = twi0_writeTo(i2c_address, &data, length, wait, sendStop); 
        if (twi_returnCode == 0) break; // error free code
        if (i>5) return 0; // give up after 5 trys
    }
    
    // A manager was found now try to read the address from it
    uint8_t txBuffer[ADDRESS_CMD_SIZE] = ADDRESS_CMD;
    length = ADDRESS_CMD_SIZE;
    sendStop = 0;  //this will cause a I2C repeated Start durring read
    twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop); 
    if (twi_returnCode != 0)
    {
        return 0; // nack failed
    }

    uint8_t rxBuffer[ADDRESS_CMD_SIZE];
    sendStop = 1;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, length, sendStop);
    if ( bytes_read != length )
    {
        return 0;
    }
    else
    {
        return (char)(rxBuffer[1]);
    }
}

// management commands 32, 33, 34 and 35 are used to analogRead ALT_I,ALT_V,PWR_I and PWR_V
int i2c_get_analogRead_from_manager(uint8_t command)
{
    if ((command<32) | (command>35)) return 0;
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR; //0x29
    uint8_t length = ANALOG_RD_CMD_SIZE;
    uint8_t wait = 1;
    uint8_t sendStop = 0; // use a repeated start after write
    uint8_t txBuffer[ANALOG_RD_CMD_SIZE] = ANALOG_RD_CMD; // init the buffer sinse it is on the stack and can have old values 
    txBuffer[0] = command; // replace the command byte
    uint8_t twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop); 
    if (twi_returnCode != 0)
    {
        return 0;
    }
    uint8_t rxBuffer[ANALOG_RD_CMD_SIZE];
    sendStop = 1;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, length, sendStop);
    if ( bytes_read != length )
    {
        return 0;
    }
    int value = ((int)(rxBuffer[1])<<8) + rxBuffer[2]; //  least significant byte is at end.
    return value;
}

// The manager has a status byte that has the following bits. 
// 0: DTR readback timeout, 1: twi transmit fail, 2: DTR readback not match
// 3: host lockout, 4. alternate power enable (ALT_EN), 5. SBC power enable (PIPWR_EN)
uint8_t i2c_read_status(void)
{ 
    uint8_t twi_returnCode;
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR;

    uint8_t txBuffer[STATUS_READ_CMD_SIZE] = STATUS_READ_CMD; //detect host shutdown comand 0x04, data place holder 0xFF;
    uint8_t length = STATUS_READ_CMD_SIZE;
    uint8_t wait = 1;
    uint8_t sendStop = 0;  //this will cause a I2C repeated Start durring read
    twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop); 
    if (twi_returnCode != 0)
    {
        return 0; // nack failed
    }
    
    // above writes data to slave, this reads data from slave
    uint8_t rxBuffer[STATUS_READ_CMD_SIZE];
    sendStop = 1;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, length, sendStop);
    if ( bytes_read != length )
    {
        return 0;
    }
    else
    {
        return rxBuffer[1];
    }
}

// The manager has a Day-Night state machine with state in low nibble and work notice in high nibble. 
// state values range from: 0..7
// bit 7:night_work lockout, 6:day_work, 5:set to see 6 and 7, 4:set to clear 6 and 7.
uint8_t i2c_daynight_state(uint8_t clear_work)
{ 
    uint8_t twi_returnCode;
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR;

    uint8_t txBuffer[DAYNIGHT_STATE_CMD_SIZE] = DAYNIGHT_STATE_CMD; //detect host shutdown comand 0x04, data place holder 0xFF;
    uint8_t length = DAYNIGHT_STATE_CMD_SIZE;
    uint8_t wait = 1;
    uint8_t sendStop = 0;  //this will cause a I2C repeated Start durring read
    if (clear_work) // default data byte has bit 5 set to show bits 6 and 7
    {
        txBuffer[1] = 0x10; // set bit 4 with 6 and 7 LOW to clear those bits 
    }
    twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop); 
    if (twi_returnCode != 0)
    {
        return 0; // nack failed
    }
    
    // above writes data to slave, this reads data from slave
    uint8_t rxBuffer[DAYNIGHT_STATE_CMD_SIZE];
    sendStop = 1;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, length, sendStop);
    if ( bytes_read != length )
    {
        return 0;
    }
    else
    {
        return rxBuffer[1];
    }
}

// management commands 52,53 and 54 are used to access managers daynight time prameters
unsigned long i2c_daynight_debounce(uint8_t command, unsigned long update_with)
{
    if ((command<52) | (command>54)) return 0;
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR; //0x29
    uint8_t length = ULONGINT_CMD_SIZE;
    uint8_t wait = 1;
    uint8_t sendStop = 0; // use a repeated start after write
    uint8_t txBuffer[ULONGINT_CMD_SIZE] = ULONGINT_CMD;
    txBuffer[0] = command; // replace the command byte
    txBuffer[3] = (uint8_t)((update_with & 0xFF000000UL)>>24);
    txBuffer[3] = (uint8_t)((update_with & 0xFF0000UL)>>16);
    txBuffer[3] = (uint8_t)((update_with & 0xFF00UL)>>8);
    txBuffer[4] = (uint8_t)(update_with & 0xFFUL);
    uint8_t twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop); 
    if (twi_returnCode != 0)
    {
        return 0;
    }
    uint8_t rxBuffer[ULONGINT_CMD_SIZE];
    sendStop = 1;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, length, sendStop);
    if ( bytes_read != length )
    {
        return 0;
    }
    unsigned long value = ((unsigned long)(rxBuffer[1]))<<24;
    value += ((unsigned long)(rxBuffer[2]))<<16;
    value += ((unsigned long)(rxBuffer[3]))<<8;
    value +=  (unsigned long)rxBuffer[4];
    return value;
}

// management commands 21 and 22 are used to access managers daynight threshold prameters
int i2c_daynight_threshold(uint8_t command, int update_with)
{
    if ((command<21) | (command>22)) return 0;
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR; //0x29
    uint8_t length = INT_CMD_SIZE;
    uint8_t wait = 1;
    uint8_t sendStop = 0; // use a repeated start after write
    uint8_t txBuffer[INT_CMD_SIZE] = INT_CMD; 
    txBuffer[0] = command; // replace the command byte
    txBuffer[1] = (uint8_t)((update_with & 0xFF00)>>8);
    txBuffer[2] = (uint8_t)(update_with & 0xFF);
    uint8_t twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop); 
    if (twi_returnCode != 0)
    {
        return 0;
    }
    uint8_t rxBuffer[INT_CMD_SIZE];
    sendStop = 1;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, length, sendStop);
    if ( bytes_read != length )
    {
        return 0;
    }
    int value = ((int)(rxBuffer[1]))<<8;
    value +=  (int)rxBuffer[2];
    return value;
}