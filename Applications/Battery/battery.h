#ifndef Battery_H
#define Battery_H

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

extern volatile BATTERYMGR_STATE_t bm_state;
extern volatile uint8_t bm_enable; // this is in bit 7 of the bm_state event sent by manager atter it is told where to send updates

extern void EnableBatMngCntl(void);
extern void ReportBatMngCntl(unsigned long serial_print_delay_milsec);
extern void BatMngLowLimit(void);
extern void BatMngHighLimit(void);
extern void BatMngHostLimit(void);

#endif // Battery_H 
