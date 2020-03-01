#ifndef twi0_h
#define twi0_h

#define TWI0_BUFFER_LENGTH 32

typedef enum TWI0_PINS_enum {
    TWI0_PINS_FLOATING,
    TWI0_PINS_PULLUP
} TWI0_PINS_t;

void twi0_init(uint32_t bitrate, TWI0_PINS_t pull_up);

uint8_t twi0_masterAsyncWrite(uint8_t slave_address, uint8_t *write_data, uint8_t bytes_to_write, uint8_t send_stop);
uint8_t twi0_masterAsyncWrite_status(void);
uint8_t twi0_masterBlockingWrite(uint8_t slave_address, uint8_t* write_data, uint8_t bytes_to_write, uint8_t send_stop);

uint8_t twi0_masterAsyncRead(uint8_t slave_address, uint8_t bytes_to_read, uint8_t send_stop);
uint8_t twi0_masterAsyncRead_bytesRead(uint8_t *read_data);
uint8_t twi0_masterBlockingRead(uint8_t slave_address, uint8_t* read_data, uint8_t bytes_to_read, uint8_t send_stop);

uint8_t twi0_fillSlaveTxBuffer(const uint8_t* slave_data, uint8_t bytes_to_send);
void twi0_registerSlaveRxCallback( void (*function)(uint8_t*, int) );
void twi0_registerSlaveTxCallback( void (*function)(void) );

#endif // twi0_h

