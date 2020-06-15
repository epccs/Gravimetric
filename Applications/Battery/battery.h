#ifndef Battery_H
#define Battery_H

typedef enum BATTERYMGR_STATE_enum {
    BATTERYMGR_STATE_START, // start power manager
    BATTERYMGR_STATE_CC_REST, // turn off alternat power so battery reading can be made
    BATTERYMGR_STATE_CC_MODE, // constant current mode, but we still need to rest every so often to measure the battery
    BATTERYMGR_STATE_PWM_MODE, // pwm starts half way between high and low limit. The off time increases up to the high limit.
    BATTERYMGR_STATE_DONE, // got to max charge, so charge is done
    BATTERYMGR_STATE_FAIL
} BATTERYMGR_STATE_t;

extern volatile BATTERYMGR_STATE_t bm_state;

extern void EnableBatMngCntl(void);
extern void ReportBatMngCntl(unsigned long serial_print_delay_milsec);
extern void BatMngLowLimit(void);
extern void BatMngHighLimit(void);

#endif // Battery_H 
