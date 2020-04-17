#ifndef ADC_burst_H
#define ADC_burst_H

// adc takes 24 clocks at 12MHz/64  or about 1536 MCU clocks,
// in addition the ISR takes over 300 MCU clocks.  
// so about 6k5 conversions per sec (12000000/(64*24 + 300)) is the best I can hope for 
// if I do 100 burst per second then that should be witin the averaging range of the 1uF cap
// so that amp hour values are good, and the math to convert the readings would be easy.
#define ADC_DELAY_MILSEC 10UL


extern void adc_burst(void);

extern unsigned long adc_started_at;
extern unsigned long accumulate_alt_ti;
extern unsigned long accumulate_alt_mega_ti;
extern unsigned long accumulate_pwr_ti;
extern unsigned long accumulate_pwr_mega_ti;

#endif // ADC_burst_H 
