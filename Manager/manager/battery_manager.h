#ifndef Battery_manager_H
#define Battery_manager_H

typedef enum BATTERYMGR_STATE_enum {
    BATTERYMGR_STATE_START, // start power manager
    BATTERYMGR_STATE_CC_REST, // turn off alternat power so battery reading can be made
    BATTERYMGR_STATE_CC_MODE, // constant current mode, but we still need to rest every so often to measure the battery
    BATTERYMGR_STATE_PWM_MODE_OFF, // pwm starts at low limit and ontime decreases up to the high limit.
    BATTERYMGR_STATE_PWM_MODE_ON, // ALT_EN is on so that the battery is charging.
    BATTERYMGR_STATE_DONE, // got to max charge, so charge is done
    BATTERYMGR_STATE_PREFAIL, // get ready for fail mode
    BATTERYMGR_STATE_FAIL // loop in fail mode
} BATTERYMGR_STATE_t;

extern BATTERYMGR_STATE_t bm_state;

extern unsigned long alt_pwm_accum_charge_time;

extern uint8_t bm_callback_address; // callback address of the application (i2c slave)
extern uint8_t bm_callback_route; // routing number to use when sending the battery manager state update 
extern uint8_t bm_callback_poke; // don't poke me... oh you reset or I am hung in the fail mode.
extern uint8_t bm_enable; // enable battery manager

extern void check_battery_manager(void);

#define ALT_PWM_PERIOD 2000
#define ALT_REST_PERIOD 10000
#define ALT_REST 250

#endif // Battery_manager_H 
