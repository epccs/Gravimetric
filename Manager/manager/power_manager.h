#ifndef Power_manager_H
#define Power_manager_H

extern uint8_t enable_alternate_power;
extern uint8_t enable_sbc_power;
extern uint16_t alt_count;

extern void check_if_alt_should_be_on(void);

#endif // Power_manager_H 
