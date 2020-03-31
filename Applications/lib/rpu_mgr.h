#ifndef Rpu_Mgr_h
#define Rpu_Mgr_h

#include "../lib/twi0_bsd.h"

// enumeraiton names for ADC from manager
typedef enum ADC_CH_MGR_enum {
    ADC_CH_MGR_ALT_I, // manager analog channel 0
    ADC_CH_MGR_ALT_V, // manager analog channel 1
    ADC_CH_MGR_PWR_I, // manager analog channel 6
    ADC_CH_MGR_PWR_V, // manager analog channel 7
    ADC_CH_MGR_MAX_NOT_A_CH // not a channel
} ADC_CH_MGR_t;

extern uint8_t twi_errorCode;

extern void i2c_ping(void);
extern uint8_t i2c_set_Rpu_shutdown(void);
extern uint8_t i2c_detect_Rpu_shutdown(void);
extern char i2c_get_Rpu_address(void);
extern int i2c_get_adc_from_manager(uint8_t channel, TWI0_LOOP_STATE_t *loop_state);
extern uint8_t i2c_read_status(void);
extern uint8_t i2c_uint8_access_cmd(uint8_t, uint8_t);
extern unsigned long i2c_ul_access_cmd(uint8_t, unsigned long);
extern int i2c_int_access_cmd(uint8_t command, int update_with, TWI0_LOOP_STATE_t *loop_state);

// values used for i2c_uint8_access_cmd
#define DAYNIGHT_STATE 23

// values used for i2c_ul_access_cmd
#define CHARGE_BATTERY_ABSORPTION 20
#define EVENING_DEBOUNCE 52
#define MORNING_DEBOUNCE 53
#define DAYNIGHT_TIMER 54

// values used for i2c_int_access_cmd
#define CHARGE_BATTERY_START 18
#define CHARGE_BATTERY_STOP 19
#define MORNING_THRESHOLD 21
#define EVENING_THRESHOLD 22


#endif // Rpu_Mgr_h
