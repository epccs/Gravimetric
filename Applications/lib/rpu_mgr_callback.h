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
extern void fnDayNightState(uint8_t*); // 1  daynight_state
extern void fnDayWork(uint8_t*); // 2 day_work_callback
extern void fnNightWork(uint8_t*); // 3 night_work_callback
// not used  // 4
// not used  // 5
// not used // 6
// not used // 7

/* Dummy function */
extern  void fnNull(uint8_t*);

// Register Callbacks 
void twi0_registerOnDayNightStateCallback( void (*function)(uint8_t data) );
void twi0_registerOnDayWorkCallback( void (*function)(uint8_t data) );
void twi0_registerOnNightWorkCallback( void (*function)(uint8_t data) );

#endif // Rpu_Mgr_Callback_H 
