#ifndef Daynight_State_H
#define Daynight_State_H

typedef enum DAYNIGHT_STATE_enum {
    DAYNIGHT_STATE_START, // Start day-night state machine
    DAYNIGHT_STATE_DAY, // day
    DAYNIGHT_STATE_EVENING_DEBOUNCE, // was day, maybe a dark cloud, or the PV panel got covered
    DAYNIGHT_STATE_NIGHTWORK, // task to at start of night, lights enabled, PV panel blocked so it does not drain battery
    DAYNIGHT_STATE_NIGHT, // night
    DAYNIGHT_STATE_MORNING_DEBOUNCE, // was night, maybe a flash light or...
    DAYNIGHT_STATE_DAYWORK, // task to at start of day, charge battery, water the garden
    DAYNIGHT_STATE_FAIL
} DAYNIGHT_STATE_t;

extern DAYNIGHT_STATE_t daynight_state; 
extern uint8_t daynight_work;
extern unsigned long dayTmrStarted;
extern uint8_t daynight_callback_address;
extern uint8_t daynight_state_callback_cmd;
extern uint8_t day_work_callback_cmd;
extern uint8_t night_work_callback_cmd;

extern void check_daynight(void);

#endif // Daynight_State_H 
