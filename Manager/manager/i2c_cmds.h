#ifndef I2C_cmds_H
#define I2C_cmds_H

#define I2C_BUFFER_LENGTH 32

#define GROUP  4
#define MGR_CMDS  8

extern uint8_t i2c0Buffer[I2C_BUFFER_LENGTH];
extern uint8_t i2c0BufferLength;

extern void receive_i2c_event(uint8_t*, int);
extern void transmit_i2c_event(void);

// Prototypes for point 2 multipoint commands
extern void fnRdMgrAddr(uint8_t*);
extern void fnRdMgrAddrQuietly(uint8_t*); // SMBus should do this
extern void fnWtMgrAddr(uint8_t*);
extern void fnRdBootldAddr(uint8_t*);
extern void fnWtBootldAddr(uint8_t*);
extern void fnRdShtdnDtct(uint8_t*);
extern void fnWtShtdnDtct(uint8_t*);
extern void fnRdStatus(uint8_t*);
extern void fnWtStatus(uint8_t*);

// Prototypes for point 2 point commands
extern void fnWtArduinMode(uint8_t*);
extern void fnRdArduinMode(uint8_t*);

// Prototypes for reserved (e.g. power management?) commands
// TBD

// Prototypes for test mode commands
extern void fnStartTestMode(uint8_t*);
extern void fnEndTestMode(uint8_t*);
extern void fnRdXcvrCntlInTestMode(uint8_t*);
extern void fnWtXcvrCntlInTestMode(uint8_t*);

/* Dummy function */
extern  void fnNull(uint8_t*);

#endif // I2C_cmds_H 
