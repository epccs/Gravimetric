#ifndef References_H
#define References_H

//References EEPROM memory usage. 
#define EE_ANALOG_BASE_ADDR 30
// each setting is at this byte offset
#define EE_ANALOG_ID 0
#define EE_ANALOG_REF_EXTERN_AVCC 2
#define EE_ANALOG_REF_INTERN_1V1 6

#define REF_EXTERN_AVCC_MAX 5500000UL
#define REF_EXTERN_AVCC_MIN 4500000UL
#define REF_INTERN_1V1_MAX 1300000UL
#define REF_INTERN_1V1_MIN 900000UL

extern uint8_t IsValidValForAvccRef(uint32_t *);
extern uint8_t IsValidValFor1V1Ref(uint32_t *);
extern uint8_t LoadAnalogRefFromEEPROM();
extern uint8_t WriteEeReferenceId();
extern uint8_t WriteEeReferenceAvcc();
extern uint8_t WriteEeReference1V1();
extern void ref2ee();

#define REF_LOADED 0
#define REF_DEFAULT 1
#define REF_AVCC_TOSAVE 2
#define REF_1V1_TOSAVE 3
extern uint8_t ref_loaded;
extern uint32_t ref_extern_avcc_uV;
extern uint32_t ref_intern_1v1_uV;

#endif // Analog_H 
