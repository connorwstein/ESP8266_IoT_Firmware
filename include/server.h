#ifndef SERVER_H
#define SERVER_H

#include "c_types.h"
#include "espconn.h"

#include "message_queue.h"

int ICACHE_FLASH_ATTR sta_server_init();
void ICACHE_FLASH_ATTR sta_server_close();

int ICACHE_FLASH_ATTR ap_server_init();
void ICACHE_FLASH_ATTR ap_server_close();

int tcpserver_send(struct espconn *conn, uint8 *buf, uint16 len, enum Memtype mem);

#endif
