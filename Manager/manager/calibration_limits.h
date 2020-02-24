#ifndef Calibration_H
#define Calibration_H

// enum ADC_CH_enum for: ADC_CH_ALT_I, ADC_CH_ALT_V, ADC_CH_PWR_I, ADC_CH_PWR_V, ADC_CHANNELS
#include "../lib/adc_bsd.h"

//EEPROM memory usage (see README.md). 
#define EE_CAL_BASE_ADDR 82
// EEPROM byte offset to each calibration value
#define EE_CAL_OFFSET 4
// number of calibration values held in eeprom
#define EE_CAL_NUM 4

// calibrations needed for channels: ALT_I, ALT_V,PWR_I,PWR_V
struct Cal_Map { // https://yarchive.net/comp/linux/typedefs.html
    float calibration; // IEEE 754 single-precision floating-point format has about 7 significant figures
};

// array of calibration that needs to fill from LoadCalFromEEPROM()
extern struct Cal_Map calMap[EE_CAL_NUM];

// map channel to calibration: ALT_I, ALT_V,PWR_I,PWR_V defined in ../lib/pins_board.h
struct Channel_Map { // https://yarchive.net/comp/linux/typedefs.html
    uint8_t cal_map; // channel number for ADC
};

const static struct Channel_Map channelMap[ADC_CHANNELS] = {
    [ADC_CH_ALT_I] = { .cal_map= 0 }, 
    [ADC_CH_ALT_V] = { .cal_map= 1 }, 
    [ADC_CH_PWR_I] = { .cal_map= 2 }, 
    [ADC_CH_PWR_V] = { .cal_map= 3 } 
};


extern uint8_t IsValidValForCal(uint8_t);
extern uint8_t IsValidValForCalChannel(void);
extern uint8_t WriteCalToEE(void);
extern void LoadCalFromEEPROM(uint8_t);
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
extern volatile uint8_t channel_with_writebit;

#endif // Calibration_H 
