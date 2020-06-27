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
#define CB_ROUTE_NULL  0
// not used //0
#define CB_ROUTE_DN_STATE  1 /* route to report daynight_state */
extern void fnDayNightState(uint8_t*); // function that receives daynight_state as an i2c slave event on the above route
#define CB_ROUTE_DN_DAYWK  2 /* route to report day work event */
extern void fnDayWork(uint8_t*); // function that receives an i2c slave event on the above route
#define CB_ROUTE_DN_NIGHTWK  3 /* route to report night work event */
extern void fnNightWork(uint8_t*); // function that receives an i2c slave event on the above route
#define CB_ROUTE_BM_STATE  4 /* route to report battery manager state */
extern void fnBatMgrState(uint8_t*); // function that receives an i2c slave event on the above route
#define CB_ROUTE_HS_STATE  5 /* route to report host shutdown manager state */
extern void fnHostShutdownState(uint8_t*); // function that receives an i2c slave event on the above route
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
