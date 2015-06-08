#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include "c_types.h"
#include "espconn.h"

void ICACHE_FLASH_ATTR Temperature_set_temperature(int temp);
void ICACHE_FLASH_ATTR Temperature_get_temperature(struct espconn *conn);

#endif
