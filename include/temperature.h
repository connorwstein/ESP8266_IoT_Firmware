#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include "c_types.h"
#include "espconn.h"

#include "device_config.h"

void ICACHE_FLASH_ATTR Temperature_get_temperature(struct espconn *conn);

int ICACHE_FLASH_ATTR Temperature_init(struct DeviceConfig *config);
int ICACHE_FLASH_ATTR Temperature_set_default_data(struct DeviceConfig *config);

#endif
