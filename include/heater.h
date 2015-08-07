#ifndef HEATER_H
#define HEATER_H

#include "c_types.h"

#include "device_config.h"

enum HeaterState {
	HEATER_OFF,
	HEATER_ON
};

void ICACHE_FLASH_ATTR Heater_turn_on();
void ICACHE_FLASH_ATTR Heater_turn_off();

int ICACHE_FLASH_ATTR Heater_init(struct DeviceConfig *config);
int ICACHE_FLASH_ATTR Heater_set_default_data(struct DeviceConfig *config);

#endif
