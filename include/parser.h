#ifndef PARSER_H
#define PARSER_H

#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"

void ICACHE_FLASH_ATTR tcpparser_process_data(char *data, uint16 len, struct espconn *conn);
void ICACHE_FLASH_ATTR udpparser_process_data(char *data, uint16 len, struct espconn *conn);

#endif
