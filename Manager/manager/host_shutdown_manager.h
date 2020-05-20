#ifndef Host_Shutdown_manager_H
#define Host_Shutdown_manager_H

// The Raspberry Pi needs to run a Shutdown script that will halt when BCM6 is pulled low.
typedef enum HOSTSHUTDOWN_STATE_enum {
    HOSTSHUTDOWN_STATE_UP, // this host is assumed to be UP and running 
    HOSTSHUTDOWN_STATE_SW_HALT, // software cmd starts with this to pull down the shutdown pin (BCM6) and cause a hault.
    HOSTSHUTDOWN_STATE_HALT, // a manual switch operation or after software
    HOSTSHUTDOWN_STATE_BM_CHK, // the battery manager enable was saved and is now disable, wait for CURR on PWR_I
    HOSTSHUTDOWN_STATE_HALTTIMEOUT_RESET_APP, // If the hault current is not seen after a timeout then fail 
    HOSTSHUTDOWN_STATE_AT_HALT_CURR, // now PWR_I is bellow the expected level, it is differnt for each model of R-Pi
    HOSTSHUTDOWN_STATE_DELAY, // once the hault has been verifed, this state is a delay in miliseconds befor removing power
    HOSTSHUTDOWN_STATE_WEARLEVELING, // when PWR_I ADC readings are stable for a period of time it may indicate that wear leveling is done (have a few grains of salt whith that). Unfortunately, wear leveling is not in the SD spec so there is not a proper way to ensure it has finished. 
    HOSTSHUTDOWN_STATE_BM_RESUME, // the battery manager status is recovered, so it will start charging if it was enabled.
    HOSTSHUTDOWN_STATE_DOWN, // the host is powerd off e.g., the managers PIPWR_EN pin has been pulled low.
    HOSTSHUTDOWN_STATE_RESTART, // restart the SBC
    HOSTSHUTDOWN_STATE_FAIL // somthing went wrong, the managers PIPWR_EN pin will be pulled low which may have damage the SD card.
} HOSTSHUTDOWN_STATE_t;

extern HOSTSHUTDOWN_STATE_t shutdown_state;

extern unsigned long shutdown_started_at; // time when BCM6 was pulled low
extern unsigned long shutdown_halt_chk_at; // time when current on PWR_I got bellow the expected level
extern unsigned long shutdown_wearleveling_done_at; // when current on PWR_I got stable for a period


extern uint8_t shutdown_callback_address; // set callback address, zero will stop sending events to application
extern uint8_t shutdown_state_callback_cmd; // command number to use with shutdown event updates.

extern void check_if_host_should_be_on(void);

#endif // Host_Shutdown_manager_H 
