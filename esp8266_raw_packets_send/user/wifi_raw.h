#ifndef WIFI_RAW
#define WIFI_RAW

#include "c_types.h"

void ICACHE_FLASH_ATTR wifi_send_raw_packet(void *buf, uint16 len);

#endif
