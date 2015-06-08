#ifndef HELPER_H
#define HELPER_H

#include "c_types.h"
#include "ip_addr.h"
#include "user_interface.h"

char * ICACHE_FLASH_ATTR separate(char *str, char sep);
void ICACHE_FLASH_ATTR strip_newline(char *str);

int ICACHE_FLASH_ATTR generate_default_ssid(char *ssid, uint8 len);

void ICACHE_FLASH_ATTR print_softap_config(const struct softap_config *config);

const char * ICACHE_FLASH_ATTR str_bssid(uint8 *bssid);
const char * ICACHE_FLASH_ATTR inet_ntoa(uint32 addr);

void ICACHE_FLASH_ATTR print_ip_info(const struct ip_info *info);

const char * ICACHE_FLASH_ATTR str_mac(uint8 *mac);

#endif
