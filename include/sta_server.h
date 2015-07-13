#ifndef STA_SERVER_H
#define STA_SERVER_H

#include "c_types.h"
#include "espconn.h"

void ICACHE_FLASH_ATTR sta_server_init_tcp();
void ICACHE_FLASH_ATTR sta_server_init_udp();

void sta_server_send_large_buffer(struct espconn *conn, uint8 *buf, uint16 len);

#endif
