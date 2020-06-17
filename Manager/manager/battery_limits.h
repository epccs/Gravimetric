#ifndef Battery_limits_H
#define Battery_limits_H

//References EEPROM memory usage. 
#define EE_BAT_LIMIT_ADDR 150
// each setting is at this byte offset
#define EE_BAT_LIMIT_OFFSET_HIGH 0
#define EE_BAT_LIMIT_OFFSET_LOW 2
#define EE_BAT_LIMIT_OFFSET_HOST 4

// 12V range
// 15.0/(((4.5)/1024.0)*(115.8/15.8))
#define BAT12_LIMIT_HIGH_MAX 466
// 13.0/(((5.5)/1024.0)*(115.8/15.8))
#define BAT12_LIMIT_HIGH_MIN 330
// 13.5/(((4.5)/1024.0)*(115.8/15.8))
#define BAT12_LIMIT_LOW_MAX 420
// 12.5/(((5.5)/1024.0)*(115.8/15.8))
#define BAT12_LIMIT_LOW_MIN 317
// 12.5/(((4.5)/1024.0)*(115.8/15.8))
#define BAT12_LIMIT_HOST_MAX 388
// 10.5/(((5.5)/1024.0)*(115.8/15.8))
#define BAT12_LIMIT_HOST_MIN 266

// 24V range
// 30.0/(((4.5)/1024.0)*(115.8/15.8))
#define BAT24_LIMIT_HIGH_MAX 932
// 26.0/(((5.5)/1024.0)*(115.8/15.8))
#define BAT24_LIMIT_HIGH_MIN 660
// 27.0/(((4.5)/1024.0)*(115.8/15.8))
#define BAT24_LIMIT_LOW_MAX 839
// 25.0/(((5.5)/1024.0)*(115.8/15.8))
#define BAT24_LIMIT_LOW_MIN 635
// 25.0/(((4.5)/1024.0)*(115.8/15.8))
#define BAT24_LIMIT_HOST_MAX 776
// 21.0/(((5.5)/1024.0)*(115.8/15.8))
#define BAT24_LIMIT_HOST_MIN 533

typedef enum BAT_LIM_enum {
    BAT_LIM_LOADED, // Limits loaded from EEPROM
    BAT_LIM_DEFAULT, // Limits have default values from source
    BAT_LIM_HIGH_TOSAVE, // i2c has set the high limit, it needs checked and if valid saved into EEPROM
    BAT_LIM_LOW_TOSAVE, // i2c has set the low limit, it needs checked and if valid saved into EEPROM
    BAT_LIM_HOST_TOSAVE, // i2c has set the host limit, it needs checked and if valid saved into EEPROM
    BAT_LIM_DELAY_LOAD // wait for ADC to read battery voltage befor selecting defaults
} BAT_LIM_t;

extern BAT_LIM_t bat_limit_loaded;
extern int battery_high_limit; // stop PWM and turn off charging when battery is above this value
extern int battery_low_limit; // start PWM when battery is above this value
extern int battery_host_limit; // turn off host when battery is bellow this value

extern uint8_t IsValidBatHighLimFor12V(int *);
extern uint8_t IsValidBatLowLimFor12V(int *);
extern uint8_t IsValidBatHostLimFor12V(int *);
extern uint8_t IsValidBatHighLimFor24V(int *);
extern uint8_t IsValidBatLowLimFor24V(int *);
extern uint8_t IsValidBatHostLimFor24V(int *);
extern uint8_t WriteEEBatHighLim();
extern uint8_t WriteEEBatLowLim();
extern uint8_t WriteEEBatHostLim();
extern uint8_t LoadBatLimitsFromEEPROM();
extern void BatLimitsFromI2CtoEE();

#endif // Battery_limits_H 
