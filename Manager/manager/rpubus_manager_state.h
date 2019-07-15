#ifndef RPUbus_manager_state_H
#define RPUbus_manager_state_H

//I2C_ADDRESS slave address is defined in the Makefile use numbers between 0x08 to 0x78
// RPU_ADDRESS is defined in the Makefile it is used as an ascii characer
// If the RPU_ADDRESS is defined as '1' then the addrss is 0x31. It may then be used as part of a textual command e.g. /1/id?
// RPU_HOST_CONNECT is defined in the Makefile and is used as default address sent on DTR pair if FTDI_nDTR toggles
// byte broadcast on DTR pair when HOST_nDTR (or HOST_nRTS) is no longer active
#define RPU_HOST_DISCONNECT ~RPU_HOST_CONNECT
// return to normal mode address sent on DTR pair
#define RPU_NORMAL_MODE 0x00
// bootload last forever, or until controller reads address from I2C
#define RPU_ARDUINO_MODE 0xFF
// test sent on DTR pair
#define RPU_START_TEST_MODE 0x01
// end test mode sent on DTR pair
#define RPU_END_TEST_MODE 0xFE

#define BOOTLOADER_ACTIVE 115000UL
#define LOCKOUT_DELAY 120000UL

// the LED is used to blink status
#define BLINK_BOOTLD_DELAY 75
#define BLINK_ACTIVE_DELAY 500
#define BLINK_LOCKOUT_DELAY 2000
#define BLINK_STATUS_DELAY 200

#define SHUTDOWN_TIME 1000

extern unsigned long blink_started_at;
extern unsigned long lockout_started_at;
extern unsigned long bootloader_started_at;
extern unsigned long shutdown_started_at;

extern uint8_t bootloader_started;
extern uint8_t host_active;
extern uint8_t localhost_active;
extern uint8_t bootloader_address; 
extern uint8_t lockout_active;
extern uint8_t uart_has_TTL;
extern uint8_t host_is_foreign;
extern uint8_t local_mcu_is_rpu_aware;
extern uint8_t rpu_address;
extern uint8_t write_rpu_address_to_eeprom;
extern uint8_t shutdown_detected;
extern uint8_t shutdown_started;
extern uint8_t arduino_mode_started;
extern uint8_t arduino_mode;
extern uint8_t test_mode_started;
extern uint8_t test_mode;
extern uint8_t transceiver_state;

// status_byt bits
#define DTR_READBACK_TIMEOUT 0
#define DTR_I2C_TRANSMIT_FAIL 1
#define DTR_READBACK_NOT_MATCH 2
#define HOST_LOCKOUT_STATUS 3

volatile extern uint8_t status_byt;

// rpubus mode setup
extern void connect_normal_mode(void);
extern void connect_bootload_mode(void);
extern void connect_lockout_mode(void);
extern void blink_on_activate(void);
extern void check_Bootload_Time(void);
extern void check_lockout(void);
extern void check_shutdown(void);

#endif // RPUbus_manager_state_H 
