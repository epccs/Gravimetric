#!/usr/bin/env python3
#Use Raspberry Pi /dev/i2c-1

# notes: https://git.kernel.org/pub/scm/utils/i2c-tools/i2c-tools.git/tree/py-smbus/smbusmodule.c

import smbus

# on a Raspberry Pi with 256k memory use 0 for /dev/i2c-0
bus = smbus.SMBus(1)

RPUBUS_MGR_I2C_ADDR = 0x2A
I2C_COMMAND_TO_TOGGLE_LED_BUILTIN_MODE = 0
data = [3]

#write the command
bus.write_i2c_block_data(RPUBUS_MGR_I2C_ADDR, I2C_COMMAND_TO_TOGGLE_LED_BUILTIN_MODE, data)

# up to 32 bytes can be buffered
num_of_bytes = len( [I2C_COMMAND_TO_TOGGLE_LED_BUILTIN_MODE] ) + len (data)
# the command is included in the echo

echo = bus.read_i2c_block_data(RPUBUS_MGR_I2C_ADDR, I2C_COMMAND_TO_TOGGLE_LED_BUILTIN_MODE, num_of_bytes)
print(echo)
