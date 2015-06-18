#ifndef WIFI_H
#define WIFI_H

#include "c_types.h"
#include "user_interface.h"
#include "wifi_user_interface.h"

int ICACHE_FLASH_ATTR start_station(const char *ssid, const char *password);
int ICACHE_FLASH_ATTR start_access_point(const char *ssid, const char *password, uint8 channel);

void ICACHE_FLASH_ATTR sta_wifi_handler(System_Event_t *event);

#endif
