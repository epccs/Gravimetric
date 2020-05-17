#ifndef Battery_manager_H
#define Battery_manager_H

typedef enum BATTERYMGR_STATE_enum {
    BATTERYMGR_STATE_START, // start power manager
    BATTERYMGR_STATE_CC_REST, // turn off alternat power so battery reading can be made
    BATTERYMGR_STATE_CC_MODE, // constant current mode, but we still need to rest every so often to measure the battery
    BATTERYMGR_STATE_PWM_MODE, // pwm starts half way between high and low limit. The off time increases up to the high limit.
    BATTERYMGR_STATE_DONE, // got to max charge, so charge is done
    BATTERYMGR_STATE_FAIL
} BATTERYMGR_STATE_t;

extern BATTERYMGR_STATE_t batmgr_state;

extern unsigned long alt_pwm_accum_charge_time;

extern uint8_t enable_bm_callback_address; // enable bm and set callback address, zero will disable and stop sending events to application
extern uint8_t battery_state_callback_cmd; // command number to use with bm event updates, zero will prevent sending events to application but bm will run.

extern void check_battery_manager(void);

#define ALT_PWM_PERIOD 2000
#define ALT_REST_PERIOD 10000
#define ALT_REST 250

#endif // Battery_manager_H 
