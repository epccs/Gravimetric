#ifndef Battery_H
#define Battery_H

extern void EnableAlt(void);
extern void AltPwrCntl(unsigned long);

extern void check_if_alt_should_be_on(uint8_t, float, float);

extern  uint8_t alt_enable;


#endif // Battery_H 
