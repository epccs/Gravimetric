#ifndef Shutdown_H
#define Shutdown_H

typedef enum HOSTSHUTDOWN_STATE_enum {
    HOSTSHUTDOWN_STATE_UP, // this host is assumed to be UP and running 
    HOSTSHUTDOWN_STATE_SW_HALT, // software cmd starts with this to pull down the shutdown pin (BCM6) and cause a hault.
    HOSTSHUTDOWN_STATE_HALT, // a manual switch operation or after software halt, next disable battery manager
    HOSTSHUTDOWN_STATE_CURR_CHK, // wait for CURR on PWR_I, then restore battery manager
    HOSTSHUTDOWN_STATE_HALTTIMEOUT_RESET_APP, // If the hault current is not seen after a timeout then fail 
    HOSTSHUTDOWN_STATE_AT_HALT_CURR, // now PWR_I is bellow the expected level, it is differnt for each model of R-Pi
    HOSTSHUTDOWN_STATE_DELAY, // once the hault has been verifed, this state is a delay in miliseconds befor removing power
    HOSTSHUTDOWN_STATE_WEARLEVELING, // when PWR_I ADC readings are stable for a period of time it may indicate that wear leveling is done (have a few grains of salt whith that). Unfortunately, wear leveling is not in the SD spec so there is not a proper way to ensure it has finished. 
    HOSTSHUTDOWN_STATE_BM_RESUME, // the battery manager status is recovered, so it will start charging if it was enabled.
    HOSTSHUTDOWN_STATE_DOWN, // the host is powerd off e.g., the managers PIPWR_EN pin has been pulled low.
    HOSTSHUTDOWN_STATE_RESTART, // restart the SBC and lockout manual shutdown switch
    HOSTSHUTDOWN_STATE_RESTART_DLY, // wait for 60 sec then enable manual shutdown switch
    HOSTSHUTDOWN_STATE_FAIL // somthing went wrong, the managers PIPWR_EN pin will be pulled low which may have damage the SD card.
} HOSTSHUTDOWN_STATE_t;

extern volatile HOSTSHUTDOWN_STATE_t hs_state; // host shutdown state

extern void EnableShutdownCntl(void);
extern void ReportShutdownCntl(unsigned long serial_print_delay_milsec);
extern void ShutdownHaltCurrLimit(void);
extern void ShutdownTTLimit(void);
extern void ShutdownDelayLimit(void);
extern void ShutdownWearlevelingLimit(void);

#endif // Shutdown_H 
