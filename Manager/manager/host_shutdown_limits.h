#ifndef Host_Shutdown_limits_H
#define Host_Shutdown_limits_H

//References EEPROM memory usage. 
#define EE_HOSTSHUTDOWN_LIMIT_ADDR 130
// each setting is at this byte offset
#define EE_HOSTSHUTDOWN_LIM_HALTCURR_LIMIT 0
#define EE_HOSTSHUTDOWN_LIM_HALT_TTL 2
#define EE_HOSTSHUTDOWN_LIM_DELAY 6
#define EE_HOSTSHUTDOWN_LIM_WEARLEVELING 10

// PWR_I halt range: adc = PWR_I/((ref/1024.0)/(0.068*50.0))
// half amp: 0.5/((4.5/1024.0)/(0.068*50.0)) 
#define HOSTSHUTDOWN_LIM_HALTCURR_LIMIT_MAX 387
// 70mA: 0.07/((4.5/1024.0)/(0.068*50.0)) 
#define HOSTSHUTDOWN_LIM_HALTCURR_LIMIT_MIN 54

// Halt timeout (e.g., Time To Live) milliseconds
#define HOSTSHUTDOWN_LIM_HALT_TTL_MAX 1000000UL
#define HOSTSHUTDOWN_LIM_HALT_TTL_MIN 10UL

// Delay after halt milliseconds
#define HOSTSHUTDOWN_LIM_DELAY_MAX 3600000UL
#define HOSTSHUTDOWN_LIM_DELAY_MIN 10UL

// Wearleveling after delay milliseconds
#define HOSTSHUTDOWN_LIM_WEARLEVELING_MAX 3600000UL
#define HOSTSHUTDOWN_LIM_WEARLEVELING_MIN 10UL

typedef enum HOSTSHUTDOWN_LIM_enum {
    HOSTSHUTDOWN_LIM_LOADED, // Limits loaded from EEPROM
    HOSTSHUTDOWN_LIM_DEFAULT, // Limits have default values from source
    HOSTSHUTDOWN_LIM_HALT_CURR_TOSAVE, // i2c has set the limit that PWR_I has to be bellow to verify SBC has shutdown
    HOSTSHUTDOWN_LIM_HALT_TTL_TOSAVE, // i2c has set the timout for halt to wait for HALT_CURR
    HOSTSHUTDOWN_LIM_DELAY_TOSAVE, // i2c has set the delay to use after hault to reduce the chance of errors caused by wearleveling
    HOSTSHUTDOWN_LIM_WEARLEVELING_TOSAVE // i2c has set the stable period on PWR_I reading after delay
} HOSTSHUTDOWN_LIM_t;

extern HOSTSHUTDOWN_LIM_t shutdown_limit_loaded;
extern int shutdown_halt_curr_limit; // befor host shutdown is done PWR_I current must be bellow this limit.
extern unsigned long shutdown_ttl_limit; // time to wait for PWR_I to be bellow shutdown_halt_curr_limit and then stable for wearleveling
extern unsigned long shutdown_delay_limit; // time to wait after droping bellow shutdown_halt_curr_limit, but befor checking wearleveling for stable readings.
extern unsigned long shutdown_wearleveling_limit; // time PWR_I must be stable for 

extern uint8_t IsValidShtDwnHaltCurr(int *);
extern uint8_t IsValidShtDwnHaltTTL(unsigned long *);
extern uint8_t IsValidShtDwnDelay(unsigned long *);
extern uint8_t IsValidShtDwnWearleveling(unsigned long *);
extern uint8_t WriteEEShtDwnHaltCurr();
extern uint8_t WriteEEShtDwnHaltTTL();
extern uint8_t WriteEEShtDwnDelay();
extern uint8_t WriteEEShtDwnWearleveling();
extern uint8_t LoadShtDwnLimitsFromEEPROM();
extern void ShtDwnLimitsFromI2CtoEE();

#endif // Host_Shutdown_limits_H 
