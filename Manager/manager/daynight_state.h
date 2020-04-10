#ifndef Daynight_State_H
#define Daynight_State_H

extern uint8_t daynight_state; 
extern uint8_t daynight_work;
extern unsigned long dayTmrStarted;
extern uint8_t daynight_callback_address;
extern uint8_t daynight_state_callback_cmd;
extern uint8_t day_work_callback_cmd;
extern uint8_t night_work_callback_cmd;


extern void check_daynight(void);

#define DAYNIGHT_START_STATE 0
#define DAYNIGHT_DAY_STATE 1
#define DAYNIGHT_EVENING_DEBOUNCE_STATE 2
#define DAYNIGHT_NIGHTWORK_STATE 3
#define DAYNIGHT_NIGHT_STATE 4
#define DAYNIGHT_MORNING_DEBOUNCE_STATE 5
#define DAYNIGHT_DAYWORK_STATE 6
#define DAYNIGHT_FAIL_STATE 7

#endif // Daynight_State_H 
