#ifndef AdcISR_h
#define AdcISR_h

// enumeraiton names for ADC_CH_<node> from schematic
typedef enum ADC_CH_enum {
    ADC_CH_ALT_I,  // PC0 has analog channel 0 to measure  alternate current with 0.018 Ohm sense and a gain of 50 amplifier
    ADC_CH_ALT_V, // PC1 has analog channel 1 to measued alternate voltage with 100k and 10k Ohm divider
    ADC_CH_NC2, // PC2 is used for MCU_IO.
    ADC_CH_NC3, // PC3 is used for MCU_IO.
    ADC_CH_NC4, // PC4 is used for MCU_IO.
    ADC_CH_NC5, // PC5 is used for MCU_IO.
    ADC_CH_PWR_I, // PE2 has analog channel 6 to measure power current with 0.068 Ohm sense and a gain of 50 amplifier
    ADC_CH_PWR_V, // PE3 is analog channel 7 to measue power voltage with 100k and 15.8k Ohm divider
    ADC_CHANNELS
} ADC_CH_t;

// Analog values range from 0 to 1023, they have 1024 slots where each 
// reperesents 1/1024 of the reference. The last slot has issues see datasheet.
// the ADC channel can be used with analogRead(ADC0)*(<referance>/1024.0)

extern volatile int adc[];
extern volatile uint8_t adc_channel;
extern volatile uint8_t ADC_auto_conversion;
extern volatile uint8_t analog_reference;

#define ISR_ADCBURST_DONE 0x7F
#define ISR_ADCBURST_START 0x00
extern volatile uint8_t adc_isr_status;

typedef enum REFERENCE_enum {
    REFERENCE_EXTERN_AVCC, // 5V SMPS can be used as referance, it changes some with load.
    REFERENCE_INTERN_1V1, // Internal 1.1V band gap which may range from .9 to 1.3V from factory but is stable once known.
    REFERENCE_OPTIONS
} REFERENCE_t;

typedef enum CAL_CH_enum {
    CAL_CH_ALT_I, // index of ALT_I calibration in map
    CAL_CH_ALT_V, // index of ALT_V calibration in map
    CAL_CH_PWR_I, // index of PWR_I calibration in map
    CAL_CH_PWR_V, // index of PWR_V calibration in map
    CAL_CH_END
} CAL_CH_t;

// ADREFSMASK is used to clear all referance select (REFS) bits befor setting the needed bits
// EXTERNAL_AVCC: connects the analog reference to AVCC power supply with capacitor on AREF pin. 
// INTERNAL_1V1: a built-in 1.1V reference with bypass capacitor on AREF pin.
// INTERNAL_2V56: a built-in 2.56V reference with bypass capacitor on AREF pin.
// INTERNAL_2V56WOBP: a built-in 2.56V reference with out a bypass capacitor on AREF pin.
// EXTERNAL_AREF: the voltage applied to the AREF pin, with band gap turned off (not recomended.)
#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#define ADREFSMASK (1<<REFS2) | (1<<REFS1) | (1<<REFS0)
#define EXTERNAL_AVCC 0
#define EXTERNAL_AREF (1<<REFS0)
#define INTERNAL_1V1 (1<<REFS1) 
#define INTERNAL_2V56WOBP (1<<REFS2) | (1<<REFS1) 
#define INTERNAL_2V56 (1<<REFS2) | (1<<REFS1) | (1<<REFS0)
#endif

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__) || defined(__AVR_ATmega324PB__)
#define ADREFSMASK (1<<REFS1) | (1<<REFS0)
#define EXTERNAL_AREF 0
#define EXTERNAL_AVCC (1<<REFS0)
#define INTERNAL_1V1 (1<<REFS1)
#define INTERNAL_2V56 (1<<REFS1) | (1<<REFS0)
#endif

#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__) || (__AVR_ATmega168P__) || defined (__AVR_ATmega168__) || defined (__AVR_ATmega328PB__)
#define ADREFSMASK (1<<REFS1) | (1<<REFS0)
#define EXTERNAL_AREF 0
#define EXTERNAL_AVCC (1<<REFS0)
#define INTERNAL_1V1 (1<<REFS1) | (1<<REFS0)
#endif

#ifndef EXTERNAL_AVCC
#   error your mcu is not supported
#endif
extern void init_ADC_single_conversion(uint8_t);

#define FREE_RUNNING 1
#define BURST_MODE 0
extern void enable_ADC_auto_conversion(uint8_t);

extern int adcAtomic(ADC_CH_t);

#endif // AdcISR_h
