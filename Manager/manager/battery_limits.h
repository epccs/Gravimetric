#ifndef Battery_limits_H
#define Battery_limits_H

//References EEPROM memory usage. 
#define EE_BAT_LIMIT_ADDR 60
// each setting is at this byte offset
#define EE_BAT_LIMIT_OFFSET_HIGH 0
#define EE_BAT_LIMIT_OFFSET_LOW 2

// 12V range
// 15.0/(((4.5)/1024.0)*(115.8/15.8))
#define BAT12_LIMIT_HIGH_MAX 466
// 13.0/(((5.5)/1024.0)*(115.8/15.8))
#define BAT12_LIMIT_HIGH_MIN 330
// 13.5/(((4.5)/1024.0)*(115.8/15.8))
#define BAT12_LIMIT_LOW_MAX 420
// 12.5/(((5.5)/1024.0)*(115.8/15.8))
#define BAT12_LIMIT_LOW_MIN 317

// 24V range
// 30.0/(((4.5)/1024.0)*(115.8/15.8))
#define BAT24_LIMIT_HIGH_MAX 932
// 26.0/(((5.5)/1024.0)*(115.8/15.8))
#define BAT24_LIMIT_HIGH_MIN 660
// 27.0/(((4.5)/1024.0)*(115.8/15.8))
#define BAT24_LIMIT_LOW_MAX 839
// 25.0/(((5.5)/1024.0)*(115.8/15.8))
#define BAT24_LIMIT_LOW_MIN 635

#define BAT_LIM_LOADED 0
#define BAT_LIM_DEFAULT 1
#define BAT_HIGH_LIM_TOSAVE 2
#define BAT_LOW_LIM_TOSAVE 3
extern uint8_t bat_limit_loaded;
extern int battery_high_limit;
extern int battery_low_limit;

extern uint8_t IsValidBatHighLimFor12V(int *);
extern uint8_t IsValidBatLowLimFor12V(int *);
extern uint8_t IsValidBatHighLimFor24V(int *);
extern uint8_t IsValidBatLowLimFor24V(int *);
extern uint8_t WriteEEBatHighLim();
extern uint8_t WriteEEBatLowLim();
extern uint8_t LoadBatLimitsFromEEPROM();
extern void BatLimitsFromI2CtoEE();

#endif // Battery_limits_H 
