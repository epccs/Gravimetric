#ifndef twi1_h
#define twi1_h

#ifndef TWI1_FREQ
#define TWI1_FREQ 100000L
#endif

#ifndef TWI1_BUFFER_LENGTH
#define TWI1_BUFFER_LENGTH 32
#endif

#define TWI1_READY 0
#define TWI1_MRX   1
#define TWI1_MTX   2
#define TWI1_SRX   3
#define TWI1_STX   4

#ifndef TWI_PULLUP
#define TWI_PULLUP 1
#endif
#ifndef TWI_FLOATING
#define TWI_FLOATING 0
#endif

void twi1_init(uint8_t);
void twi1_disable(void);
void twi1_setAddress(uint8_t);
void twi1_setFrequency(uint32_t);
uint8_t twi1_readFrom(uint8_t, uint8_t*, uint8_t, uint8_t);
uint8_t twi1_writeTo(uint8_t, uint8_t*, uint8_t, uint8_t, uint8_t);
uint8_t twi1_transmit(const uint8_t*, uint8_t);
void twi1_attachSlaveRxEvent( void (*)(uint8_t*, int) );
void twi1_attachSlaveTxEvent( void (*)(void) );
void twi1_reply(uint8_t);
void twi1_stop(void);
void twi1_releaseBus(void);

#endif

