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

extern uint8_t mgr_twiErrorCode;

extern void i2c_ping(void);
extern uint8_t i2c_set_Rpu_shutdown(void);
extern uint8_t i2c_detect_Rpu_shutdown(void);
extern char i2c_get_Rpu_address(void);
extern int i2c_get_adc_from_manager(uint8_t channel, TWI0_LOOP_STATE_t *loop_state);
extern uint8_t i2c_read_status(void);
extern void i2c_daynight_cmd(uint8_t my_callback_addr);
extern void i2c_battery_cmd(uint8_t my_callback_addr);
extern void i2c_shutdown_cmd(uint8_t my_callback_addr, uint8_t up);
extern unsigned long i2c_ul_access_cmd(uint8_t command, unsigned long update_with, TWI0_LOOP_STATE_t *loop_state);
extern unsigned long i2c_ul_rwoff_access_cmd(uint8_t command, uint8_t rw_offset, unsigned long update_with, TWI0_LOOP_STATE_t *loop_state);
extern int i2c_int_access_cmd(uint8_t command, int update_with, TWI0_LOOP_STATE_t *loop_state);
extern int i2c_int_rwoff_access_cmd(uint8_t command, uint8_t rw_offset, int update_with, TWI0_LOOP_STATE_t *loop_state);
float i2c_float_access_cmd(uint8_t command, uint8_t select, float *update_with, TWI0_LOOP_STATE_t *loop_state);

// values used for i2c_*_rwoff_access_cmd
#define RW_READ_BIT 0x00
#define RW_WRITE_BIT 0x80

// values used for i2c_uint8_access_cmd
#define DAYNIGHT_STATE 23

// values used for i2c_ul_access_cmd
#define CHARGE_BATTERY_PWM 20
#define EVENING_DEBOUNCE 52
#define MORNING_DEBOUNCE 53
#define DAYNIGHT_TIMER 54

// values used for i2c_ul_rwoff_access_cmd
#define SHUTDOWN_UL_CMD 6
#define SHUTDOWN_TTL_OFFSET 0
#define SHUTDOWN_DELAY_OFFSET 1
#define SHUTDOWN_WEARLEVEL_OFFSET 2
#define SHUTDOWN_KRUNTIME 3
#define SHUTDOWN_STARTED_AT 4
#define SHUTDOWN_HALT_CHK_AT 5
#define SHUTDOWN_WEARLVL_DONE_AT 6

// values used for i2c_int_access_cmd
#define CHARGE_BATTERY_LOW 18
#define CHARGE_BATTERY_HIGH 19
#define MORNING_THRESHOLD 21
#define EVENING_THRESHOLD 22

// values used for i2c_int_rwoff_access_cmd
#define SHUTDOWN_INT_CMD 5
#define SHUTDOWN_HALT_CURR_OFFSET 0

#endif // Rpu_Mgr_h
