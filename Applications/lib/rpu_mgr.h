#ifndef Rpu_Mgr_h
#define Rpu_Mgr_h

extern uint8_t i2c_set_Rpu_shutdown(void);
extern uint8_t i2c_detect_Rpu_shutdown(void);
extern char i2c_get_Rpu_address(void);
extern int i2c_get_analogRead_from_manager(uint8_t);

// commands used for i2c_get_analogRead_from_manager
#define ALT_I 32
#define ALT_V 33
#define PWR_I 34
#define PWR_V 35

#endif // Rpu_Mgr_h
