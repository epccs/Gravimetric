#ifndef Rpu_Mgr_h
#define Rpu_Mgr_h

extern uint8_t set_Rpu_shutdown(void);
extern uint8_t detect_Rpu_shutdown(void);
extern char get_Rpu_address(void);
extern int get_adc_from_328pb(uint8_t);

// commands used for get_adc_from_328pb
#define ALT_I 32
#define ALT_V 33
#define PWR_I 34
#define PWR_V 35

#endif // Rpu_Mgr_h
