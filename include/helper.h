#ifndef HELPER_H
#define HELPER_H

#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "ip_addr.h"
#include "user_interface.h"

char * ICACHE_FLASH_ATTR separate(char *str, char sep);
void ICACHE_FLASH_ATTR strip_newline(char *str);

int ICACHE_FLASH_ATTR generate_default_ssid(char *ssid, uint8 len);

void ICACHE_FLASH_ATTR print_softap_config(const struct softap_config *config);

const char * ICACHE_FLASH_ATTR str_bssid(uint8 *bssid);
const char * ICACHE_FLASH_ATTR inet_ntoa(uint32 addr);

void ICACHE_FLASH_ATTR print_ip_info(const struct ip_info *info);

int ICACHE_FLASH_ATTR start_station(const char *ssid, const char *password);
int ICACHE_FLASH_ATTR start_access_point(const char *ssid, const char *password, uint8 channel);
void ICACHE_FLASH_ATTR sta_wifi_handler(System_Event_t *event);

const char * ICACHE_FLASH_ATTR str_mac(uint8 *mac);

int ICACHE_FLASH_ATTR read_from_flash(uint32 *data, uint32 size);
int ICACHE_FLASH_ATTR write_to_flash(uint32 *data, uint32 size);
int ICACHE_FLASH_ATTR is_flash_used();

#define DEVICE_CONFIG_BUF_LEN 64

struct DeviceConfig {
	char device_name[32];
	char device_type[32];
};

int ICACHE_FLASH_ATTR save_device_config(const struct DeviceConfig *conf);
int ICACHE_FLASH_ATTR read_device_config(struct DeviceConfig *conf);

#endif
