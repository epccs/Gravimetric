/*
  RPU bus manager functions
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

#define RPU_BUS_MSTR_CMD_SZ 2
#define I2C_ADDR_OF_BUS_MGR 0x29

uint8_t i2c_set_Rpu_shutdown(void)
{ 
    uint8_t twi_returnCode;

    //uint8_t RPU_mgr_i2c_address = I2C_ADDR_OF_BUS_MGR;

    // ping I2C for an RPU bus manager
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR;
    uint8_t data = 0;
    uint8_t length = 0;
    uint8_t wait = 1;
    uint8_t sendStop = 1;
    twi_returnCode = twi0_writeTo(i2c_address, &data, length, wait, sendStop); 
    
    if (twi_returnCode != 0)
    { 
        return 0; //nack failed
    }
    
    // An RPU bus manager was found now try to set the host shutdown command byte, this should cause the bus manager to pull down its ICP1 pin
    // note the first byte is command, second is for that data (it will size the reply from what was sent)
    uint8_t txBuffer[RPU_BUS_MSTR_CMD_SZ] = {0x05,0x01}; //host shutdown comand 0x05, data 0x01;
    length = RPU_BUS_MSTR_CMD_SZ;
    sendStop = 0;  //this will cause a I2C repeated Start durring read
    twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop); 
    if (twi_returnCode != 0)
    {
        return 0; // nack failed
    }
    
    // above writes data to slave, this reads data from slave
    uint8_t rxBuffer[RPU_BUS_MSTR_CMD_SZ];
    sendStop = 1;
    uint8_t quantity = RPU_BUS_MSTR_CMD_SZ;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, quantity, sendStop);
    if ( bytes_read != quantity )
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

uint8_t i2c_detect_Rpu_shutdown(void)
{ 
    uint8_t twi_returnCode;

    // uint8_t RPU_mgr_i2c_address = I2C_ADDR_OF_BUS_MGR;

    // ping I2C for an RPU bus manager
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR;
    uint8_t data = 0;
    uint8_t length = 0;
    uint8_t wait = 1;
    uint8_t sendStop = 1;
    twi_returnCode = twi0_writeTo(i2c_address, &data, length, wait, sendStop); 
    
    if (twi_returnCode != 0)
    { 
        return 0; //nack failed
    }
    
    // An RPU bus manager was found now try to set the host shutdown command byte, this should cause the bus manager to pull down its ICP1 pin
    // note the first byte is command, second is for that data (it will size the reply from what was sent)
    uint8_t txBuffer[RPU_BUS_MSTR_CMD_SZ] = {0x04,0xFF}; //detect host shutdown comand 0x04, data place holder 0xFF;
    length = RPU_BUS_MSTR_CMD_SZ;
    sendStop = 0;  //this will cause a I2C repeated Start durring read
    twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop); 
    if (twi_returnCode != 0)
    {
        return 0; // nack failed
    }
    
    // above writes data to slave, this reads data from slave
    uint8_t rxBuffer[RPU_BUS_MSTR_CMD_SZ];
    sendStop = 1;
    uint8_t quantity = RPU_BUS_MSTR_CMD_SZ;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, quantity, sendStop);
    if ( bytes_read != quantity )
    {
        return 0;
    }
    else
    {
        return rxBuffer[1]; // All that for one byte, Again I am not amused. 
    }
}

/* i2c command 0 gives RPUbus address
*/
char i2c_get_Rpu_address(void)
{ 
    uint8_t twi_returnCode;

    // uint8_t RPU_mgr_i2c_address = I2C_ADDR_OF_BUS_MGR;

    // ping I2C for an RPU bus manager
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR;
    uint8_t data = 0;
    uint8_t length = 0;
    uint8_t wait = 1;
    uint8_t sendStop = 1;
    for (uint8_t i =0;1; i++)
    {
        twi_returnCode = twi0_writeTo(i2c_address, &data, length, wait, sendStop); 
        if (twi_returnCode == 0) break; // error free code
        if (i>5) return 0; // give up after 5 trys
    }
    
    // An RPU bus manager was found now try to read the bus address from it
    // note the first byte is command, second is for that data (it will size the reply from what was sent)
    uint8_t txBuffer[RPU_BUS_MSTR_CMD_SZ] = {0x00,0x00}; //comand 0x00 should Read Shield RPU addr;
    length = RPU_BUS_MSTR_CMD_SZ;
    sendStop = 0;  //this will cause a I2C repeated Start durring read
    for (uint8_t i =0;1; i++)
    {
        twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop); 
        if (twi_returnCode == 0) break;
        if (i>5) return 0; // give up after 5 trys
    }
    uint8_t rxBuffer[RPU_BUS_MSTR_CMD_SZ];
    sendStop = 1;
    uint8_t quantity = RPU_BUS_MSTR_CMD_SZ;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, quantity, sendStop);
    if ( bytes_read != quantity )
    {
        return 0;
    }
    else
    {
        return (char)(rxBuffer[1]);
    }
}

/* i2c management commands 32, 33, 34 and 35 are used to read ALT_I,ALT_V,PWR_I and PWR_V
*/
int i2c_get_analogRead_from_manager(uint8_t command)
{
    if ((command<32) | (command>35)) return 0;
    uint8_t i2c_address = I2C_ADDR_OF_BUS_MGR; //0x29
    uint8_t length = 3;
    uint8_t wait = 1;
    uint8_t sendStop = 0; // use a repeated start after write
    uint8_t txBuffer[3] = {0x00,0xFF,0xFF}; // init the buffer sinse it is on the stack and can have old values 
    txBuffer[0] = command; // replace the command byte (which can not be put in flash 
    uint8_t twi_returnCode = twi0_writeTo(i2c_address, txBuffer, length, wait, sendStop); 
    if (twi_returnCode != 0)
    {
        return 0;
    }
    uint8_t rxBuffer[3] = {0x00,0x00,0x00};
    sendStop = 1;
    uint8_t bytes_read = twi0_readFrom(i2c_address, rxBuffer, length, sendStop);
    if ( bytes_read != length )
    {
        return 0;
    }
    int value = ((int)(rxBuffer[1])<<8) + rxBuffer[2]; //  least significant byte is at end.
    return value;
}