#ifndef STA_SERVER_H
#define STA_SERVER_H

#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"

void ICACHE_FLASH_ATTR sta_server_init_tcp();
void ICACHE_FLASH_ATTR sta_server_init_udp();

#endif
