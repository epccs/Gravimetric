#ifndef Rpu_Mgr_h
#define Rpu_Mgr_h

extern uint8_t i2c_set_Rpu_shutdown(void);
extern uint8_t i2c_detect_Rpu_shutdown(void);
extern char i2c_get_Rpu_address(void);
extern int i2c_get_analogRead_from_manager(uint8_t);
extern uint8_t i2c_read_status(void);
extern uint8_t i2c_uint8_access_cmd(uint8_t, uint8_t);
extern unsigned long i2c_ul_access_cmd(uint8_t, unsigned long);
extern int i2c_int_access_cmd(uint8_t, int);

// values used for i2c_get_analogRead_from_manager
#define ALT_I 32
#define ALT_V 33
#define PWR_I 34
#define PWR_V 35

// values used for i2c_uint8_access_cmd
#define DAYNIGHT_STATE 23

// values used for i2c_ul_access_cmd
#define EVENING_DEBOUNCE 52
#define MORNING_DEBOUNCE 53
#define DAYNIGHT_TIMER 54

// values used for i2c_int_access_cmd
#define MORNING_THRESHOLD 21
#define EVENING_THRESHOLD 22


#endif // Rpu_Mgr_h
