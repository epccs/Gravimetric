#ifndef Calibration_H
#define Calibration_H

// use ADC_CH_enum for: ADC_CH_ALT_I, ADC_CH_ALT_V, ADC_CH_PWR_I, ADC_CH_PWR_V, ADC_CHANNELS
// use CAL_CH_enum for: CAL_CH_ALT_I, CAL_CH_ALT_V, CAL_CH_PWR_I, CAL_CH_PWR_V, CAL_CH_END
#include "../lib/adc_bsd.h"

//EEPROM memory usage (see README.md). 
#define EE_CAL_BASE_ADDR 82
// EEPROM byte offset to each calibration value
#define EE_CAL_OFFSET 4
// number of calibration values held in eeprom
// use enum CAL_CH_END for array size

// calibrations needed for channels: ALT_I, ALT_V,PWR_I,PWR_V
struct Cal_Map { // https://yarchive.net/comp/linux/typedefs.html
    float calibration; // IEEE 754 single-precision floating-point format has about 7 significant figures
};

// array of calibration that needs to fill from LoadCalFromEEPROM()
extern struct Cal_Map calMap[ADC_ENUM_END];

// map channel to calibration: ALT_I, ALT_V,PWR_I,PWR_V defined in ../lib/pins_board.h
struct ChannelToCal_Map { // https://yarchive.net/comp/linux/typedefs.html
    ADC_ENUM_t cal_map; // map to calibration for ADC channel
};

const static struct ChannelToCal_Map channelToCalMap[ADC_CHANNELS] = {
    [ADC_CH_ALT_I] = { .cal_map= ADC_ENUM_ALT_I }, 
    [ADC_CH_ALT_V] = { .cal_map= ADC_ENUM_ALT_V }, 
    [ADC_CH_PWR_I] = { .cal_map= ADC_ENUM_PWR_I }, 
    [ADC_CH_PWR_V] = { .cal_map= ADC_ENUM_PWR_V } 
};

extern uint8_t IsValidValForCal(ADC_ENUM_t);
extern uint8_t IsValidValForCalChannel(void);
extern uint8_t WriteCalToEE(void);
extern void LoadCalFromEEPROM(ADC_ENUM_t);
extern void ChannelCalFromI2CtoEE(void);

#define CAL_CLEAR 0
#define CAL_0_DEFAULT 0x01
#define CAL_1_DEFAULT 0x02
#define CAL_2_DEFAULT 0x04
#define CAL_3_DEFAULT 0x08
#define CAL_0_TOSAVE 0x10
#define CAL_1_TOSAVE 0x20
#define CAL_2_TOSAVE 0x40
#define CAL_3_TOSAVE 0x80
extern volatile uint8_t cal_loaded;

#define CAL_CHANNEL_WRITEBIT 0x80 
#define CAL_CHANNEL_MASK 0x7F
extern volatile uint8_t adc_enum_with_writebit;

#endif // Calibration_H 
