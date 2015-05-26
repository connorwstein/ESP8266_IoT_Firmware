#ifndef AP_SERVER_H
#define AP_SERVER_H

#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"

void ICACHE_FLASH_ATTR ap_server_recv_cb(void *arg, char *pdata, unsigned short len);
void ICACHE_FLASH_ATTR ap_server_sent_cb(void *arg);
void ICACHE_FLASH_ATTR ap_server_connect_cb(void *arg);
void ICACHE_FLASH_ATTR ap_server_reconnect_cb(void *arg, sint8 err);
void ICACHE_FLASH_ATTR ap_server_disconnect_cb(void *arg);

int ICACHE_FLASH_ATTR ap_server_init();

#endif
