#ifndef CAMERA_H
#define CAMERA_H

#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"

int ICACHE_FLASH_ATTR camera_reset();
int ICACHE_FLASH_ATTR camera_take_picture();
int ICACHE_FLASH_ATTR camera_read_size(uint16 *size_p);
int ICACHE_FLASH_ATTR camera_read_content(uint16 init_addr, uint16 data_len,
					uint16 spacing_int, struct espconn *rep_conn);

#endif
