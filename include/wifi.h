#ifndef WIFI_H
#define WIFI_H

#include "c_types.h"

bool ICACHE_FLASH_ATTR has_been_connected_as_station();
bool ICACHE_FLASH_ATTR has_received_connect_instruction();

void ICACHE_FLASH_ATTR set_connected_as_station(bool val);
void ICACHE_FLASH_ATTR set_received_connect_instruction(bool val);

bool ICACHE_FLASH_ATTR in_locator_mode();
void ICACHE_FLASH_ATTR start_locator_mode();
void ICACHE_FLASH_ATTR stop_locator_mode();
void ICACHE_FLASH_ATTR locator_set_low_power(uint8 val);
void ICACHE_FLASH_ATTR locator_set_high_power(uint8 val);

int ICACHE_FLASH_ATTR start_station(const char *ssid, const char *password);
int ICACHE_FLASH_ATTR start_access_point(const char *ssid, const char *password, uint8 channel);

void ICACHE_FLASH_ATTR wifi_init();

#endif
