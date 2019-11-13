#ifndef DayNight_limits_H
#define DayNight_limits_H

//References EEPROM memory usage. 
#define EE_DAYNIGHT_ADDR 70
// each setting is at this byte offset, threshold is 2 bytes, debounce is 4 bytes
#define EE_DAYNIGHT_MORNING_THRESHOLD_OFFSET 0
#define EE_DAYNIGHT_EVENING_THRESHOLD_OFFSET 2
#define EE_DAYNIGHT_MORNING_DEBOUNCE_OFFSET 4
#define EE_DAYNIGHT_EVENING_DEBOUNCE_OFFSET 8

// 12V range (set evening first, then set morning at least 25 higher)
// 10.0/(((4.5)/1024.0)*(110.0/10.0))
#define PV12_MORNING_THRESHOLD_MAX 207
// 3.5/(((5.5)/1024.0)*(110.0/10.0))
#define PV12_MORNING_THRESHOLD_MIN 59
// 5.0/(((4.5)/1024.0)*(110.0/10.0))
#define PV12_EVENING_THRESHOLD_MAX 104
// 1.5/(((5.5)/1024.0)*(110.0/10.0))
#define PV12_EVENING_THRESHOLD_MIN 25

// 24V range
// 20.0/(((4.5)/1024.0)*(110.0/10.0))
#define PV24_MORNING_THRESHOLD_MAX 414
// 7.0/(((5.5)/1024.0)*(110.0/10.0))
#define PV24_MORNING_THRESHOLD_MIN 118
// 10.0/(((4.5)/1024.0)*(110.0/10.0))
#define PV24_EVENING_THRESHOLD_MAX 207
// 3.0/(((5.5)/1024.0)*(110.0/10.0))
#define PV24_EVENING_THRESHOLD_MIN 50

//debounce is measured in millis, default is 20 min (e.g. 1,200,000 millis)
// 60.0*60*1000 # 60 min
#define EVENING_DEBOUNCE_MAX 3600000UL
// 8.0*1000 # 8 sec
#define EVENING_DEBOUNCE_MIN 8000UL
// 60.0*60*1000 # 60 min
#define MORNING_DEBOUNCE_MAX 3600000UL
// 8.0*1000 # 8 sec
#define MORNING_DEBOUNCE_MIN 8000UL

#define DAYNIGHT_VALUES_LOADED 0
#define DAYNIGHT_VALUES_DEFAULT 1
#define DAYNIGHT_MORNING_THRESHOLD_TOSAVE 2
#define DAYNIGHT_EVENING_THRESHOLD_TOSAVE 3
#define DAYNIGHT_MORNING_DEBOUNCE_TOSAVE 4
#define DAYNIGHT_EVENING_DEBOUNCE_TOSAVE 5
extern uint8_t daynight_values_loaded;
extern int daynight_morning_threshold;
extern int daynight_evening_threshold;
extern unsigned long daynight_morning_debounce;
extern unsigned long daynight_evening_debounce;

extern uint8_t IsValidMorningThresholdFor12V(int *);
extern uint8_t IsValidEveningThresholdFor12V(int *);
extern uint8_t IsValidMorningThresholdFor24V(int *);
extern uint8_t IsValidEveningThresholdFor24V(int *);
extern uint8_t IsValidMorningDebounce(unsigned long *);
extern uint8_t IsValidEveningDebounce(unsigned long *);
extern uint8_t WriteEEMorningThreshold();
extern uint8_t WriteEEEveningThreshold();
extern uint8_t WriteEEMorningDebounce();
extern uint8_t WriteEEEveningDebounce();
extern uint8_t LoadDayNightValuesFromEEPROM();
extern void DayNightValuesFromI2CtoEE();

#endif // DayNight_limits_H 
