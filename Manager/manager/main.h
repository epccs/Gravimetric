#ifndef Main_H
#define Main_H

// managers talk on DTR pair at 250kbps
#define DTR_BAUD 250000UL
// I2C connection: applicaion MCU talks as master to manager as slave 
#define I2C0_ADDRESS 0x29
// SMBus connection: SBC is master, manager is slave 
#define I2C1_ADDRESS 0x2A
// default addres (ascii '1') used if EEPROM not set. Use I2C command 0 to set
#define RPU_ADDRESS 0x31
// default point to point (or bootload) address used when local host connects (e.g., the serial line RTS goes low).
#define RPU_HOST_CONNECT 0x30
// byte broadcast on DTR pair when HOST_nRTS is no longer active
#define RPU_HOST_DISCONNECT ~RPU_HOST_CONNECT

#endif // Main_H 
