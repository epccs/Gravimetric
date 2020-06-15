#ifndef Rpu_Mgr_Callback_H
#define Rpu_Mgr_Callback_H

#define I2C_BUFFER_LENGTH 32

#define GROUP  1
#define MGR_CMDS  8

extern uint8_t i2c0Buffer[I2C_BUFFER_LENGTH];
extern uint8_t i2c0BufferLength;

extern void receive_i2c_event(uint8_t*, uint8_t);
extern void transmit_i2c_event(void);

// Prototypes for callbacks from manager where the
// manager becomes an I2C master and sends to this slave addresss
// not used //0
extern void fnDayNightState(uint8_t*); // to get daynight_state set daynight_callback_address and daynight_state_callback_cmd to command number 1
extern void fnDayWork(uint8_t*); // to get a day work event enable daynight_callback_address and set day_work_callback_cmd to command number 2
extern void fnNightWork(uint8_t*); // to get a night work event enable daynight_callback_address and set night_work_callback_cmd to command number 3
extern void fnBatMgrState(uint8_t*);  // to get bm_state enable_bm_callback_address and set battery_state_callback_cmd to command number 4
extern void fnHostShutdownState(uint8_t*);  // to get hs_state shutdown_callback_address and set shutdown_state_callback_cmd to command number 5
// not used // 6
// not used // 7

/* Dummy function */
extern  void fnNull(uint8_t*);

// Register Callbacks 
void twi0_registerOnDayNightStateCallback( void (*function)(uint8_t data) );
void twi0_registerOnDayWorkCallback( void (*function)(uint8_t data) );
void twi0_registerOnNightWorkCallback( void (*function)(uint8_t data) );
void twi0_registerOnBatMgrStateCallback( void (*function)(uint8_t data) );
void twi0_registerOnHostShutdownStateCallback( void (*function)(uint8_t data) );


#endif // Rpu_Mgr_Callback_H 
