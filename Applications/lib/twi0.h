#ifndef twi0_h
#define twi0_h

#ifndef TWI0_FREQ
#define TWI0_FREQ 100000L
#endif

#ifndef TWI0_BUFFER_LENGTH
#define TWI0_BUFFER_LENGTH 32
#endif

#define TWI0_READY 0
#define TWI0_MRX   1
#define TWI0_MTX   2
#define TWI0_SRX   3
#define TWI0_STX   4

#ifndef TWI_PULLUP
#define TWI_PULLUP 1
#endif
#ifndef TWI_FLOATING
#define TWI_FLOATING 0
#endif

void twi0_init(uint8_t);
void twi0_disable(void);
void twi0_setAddress(uint8_t);
void twi0_setFrequency(uint32_t);
uint8_t twi0_readFrom(uint8_t, uint8_t*, uint8_t, uint8_t);
uint8_t twi0_writeTo(uint8_t, uint8_t*, uint8_t, uint8_t, uint8_t);
uint8_t twi0_transmit(const uint8_t*, uint8_t);
void twi0_attachSlaveRxEvent( void (*)(uint8_t*, int) );
void twi0_attachSlaveTxEvent( void (*)(void) );
void twi0_reply(uint8_t);
void twi0_stop(void);
void twi0_releaseBus(void);

#endif // twi0_h

