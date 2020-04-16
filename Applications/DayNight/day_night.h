#pragma once
//#ifndef DayNight_H
//#define DayNight_H

extern volatile uint8_t daynight_state;
extern int daynight_morning_threshold;
extern int daynight_evening_threshold;
extern unsigned long daynight_morning_debounce;
extern unsigned long daynight_evening_debounce;

extern void Day(unsigned long);
extern void check_daynight_state(void);
extern void check_manager_status(void);

extern void daynight_state_event(uint8_t data);
extern void day_work_event(uint8_t data);
extern void night_work_event(uint8_t data);

#define DAYNIGHT_START_STATE 0
#define DAYNIGHT_DAY_STATE 1
#define DAYNIGHT_EVENING_DEBOUNCE_STATE 2
#define DAYNIGHT_NIGHTWORK_STATE 3
#define DAYNIGHT_NIGHT_STATE 4
#define DAYNIGHT_MORNING_DEBOUNCE_STATE 5
#define DAYNIGHT_DAYWORK_STATE 6
#define DAYNIGHT_FAIL_STATE 7

//#endif // DayNight_H 
