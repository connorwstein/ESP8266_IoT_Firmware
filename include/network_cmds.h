#ifndef NETWORK_CMDS
#define NETWORK_CMDS

#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"

void ICACHE_FLASH_ATTR go_back_to_ap();
void ICACHE_FLASH_ATTR connect_to_network(const char *ssid, const char *password);
void ICACHE_FLASH_ATTR udp_send_deviceinfo(struct espconn *conn);

#endif
