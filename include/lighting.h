#ifndef LIGHTING_H
#define LIGHTING_H

#include "c_types.h"
#include "espconn.h"

enum LightState {
	LIGHT_OFF,
	LIGHT_ON
};

void ICACHE_FLASH_ATTR Lighting_toggle_light();
void ICACHE_FLASH_ATTR Lighting_get_light(struct espconn *conn);

#endif
