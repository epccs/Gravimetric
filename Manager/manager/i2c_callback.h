#ifndef I2C_Callback_h
#define I2C_Callback_h

#include "../lib/twi0_bsd.h"

extern uint8_t twi_errorCode;
extern uint8_t i2c_is_in_use;

extern void i2c_callback(uint8_t i2c_address, uint8_t command, uint8_t data,TWI0_LOOP_STATE_t *loop_state);
uint8_t i2c_callback_waiting(TWI0_LOOP_STATE_t *loop_state);

#endif // I2C_Callback_h
