#ifndef LIGHTING_H
#define LIGHTING_H

#include "c_types.h"
#include "espconn.h"

#include "device_config.h"

enum LightState {
	LIGHT_OFF,
	LIGHT_ON
};

void ICACHE_FLASH_ATTR Lighting_toggle_light();
void ICACHE_FLASH_ATTR Lighting_get_light(struct espconn *conn);

void ICACHE_FLASH_ATTR Lighting_dim(uint8 intensity);

int ICACHE_FLASH_ATTR Lighting_init(struct DeviceConfig *config);
int ICACHE_FLASH_ATTR Lighting_set_default_data(struct DeviceConfig *config);

#endif
