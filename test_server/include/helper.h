#ifndef HELPER_H
#define HELPER_H

#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "ip_addr.h"
#include "user_interface.h"

char * ICACHE_FLASH_ATTR separate(char *str, char sep);
void ICACHE_FLASH_ATTR strip_newline(char *str);

void ICACHE_FLASH_ATTR print_softap_config(const struct softap_config *config);

const char * ICACHE_FLASH_ATTR str_bssid(uint8 *bssid);
const char * ICACHE_FLASH_ATTR inet_ntoa(uint32 addr);

void ICACHE_FLASH_ATTR print_ip_info(const struct ip_info *info);

int ICACHE_FLASH_ATTR start_station(const char *ssid, const char *password);
int ICACHE_FLASH_ATTR start_access_point(const char *ssid, const char *password, uint8 channel);

#endif
