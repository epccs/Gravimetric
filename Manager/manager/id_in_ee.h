#ifndef ID_in_ee_H
#define ID_in_ee_H

// If this ID is matched in EEPROM then the rpu_address is taken from EEPROM
#define EE_RPU_IDMAX 10

#define EE_RPU_ID 40
#define EE_RPU_ADDRESS 50

extern void save_rpu_addr_state(void);
extern uint8_t check_for_eeprom_id(void);

#endif // ID_in_ee_H 
