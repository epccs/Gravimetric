#ifndef References_H
#define References_H

// use REFERENCE_enum for: EXTERN_AVCC, INTERN_1V1, REFERENCE_OPTIONS
#include "../lib/adc_bsd.h"

//EEPROM memory usage (see README.md). 
#define EE_REF_BASE_ADDR 30
// EEPROM byte offset to each reference value
#define EE_REF_OFFSET 4
// a reference value is held in eeprom for each enum (e.g., EXTERN_AVCC, INTERN_1V1)
// use enum REFERENCE_OPTIONS for array size

struct Ref_Map { 
    // https://yarchive.net/comp/linux/typedefs.html
    float reference; // IEEE 754 single-precision floating-point format has about 7 significant figures
};

// array of references (index with enum REFERENCE_enum) that needs to fill from LoadRefFromEEPROM()
extern struct Ref_Map refMap[REFERENCE_OPTIONS];

extern uint8_t IsValidValForRef(REFERENCE_t);
extern uint8_t WriteRefToEE();
extern void LoadRefFromEEPROM(REFERENCE_t);
extern void ReferanceFromI2CtoEE();

#define REF_CLEAR 0
#define REF_0_DEFAULT 0x01
#define REF_1_DEFAULT 0x02
#define REF_0_TOSAVE 0x10
#define REF_1_TOSAVE 0x20
extern volatile uint8_t ref_loaded;

#define REF_SELECT_WRITEBIT 0x80 
#define REF_SELECT_MASK 0x7F
extern volatile uint8_t ref_select_with_writebit;

#endif // Analog_H 
