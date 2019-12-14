#ifndef Power_manager_H
#define Power_manager_H

extern uint8_t enable_alternate_power;
extern unsigned long alt_pwm_accum_charge_time;

extern void check_if_alt_should_be_on(void);

#define ALT_PWM_PERIOD 2000
#define ALT_REST_PERIOD 10000
#define ALT_REST 250

#endif // Power_manager_H 
