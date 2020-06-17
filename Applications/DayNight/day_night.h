#ifndef DayNight_H
#define DayNight_H

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

extern volatile DAYNIGHT_STATE_t daynight_state;

extern void dnReport(unsigned long);
extern void dnMorningThreshold(void);
extern void dnEveningThreshold(void);
extern void dnMorningDebounce(void);
extern void dnEveningDebounce(void);

extern void check_daynight_state(void);
extern void check_manager_status(void);

extern void daynight_state_event(uint8_t data);
extern void day_work_event(uint8_t data);
extern void night_work_event(uint8_t data);

#endif // DayNight_H 
