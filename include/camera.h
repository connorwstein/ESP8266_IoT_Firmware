#ifndef CAMERA_H
#define CAMERA_H

#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"

#include "device_config.h"

int ICACHE_FLASH_ATTR Camera_reset();
int ICACHE_FLASH_ATTR Camera_take_picture();
int ICACHE_FLASH_ATTR Camera_read_size(uint16 *size_p);
int ICACHE_FLASH_ATTR Camera_read_content(uint16 init_addr, uint16 data_len,
						uint16 spacing_int, struct espconn *rep_conn);
int ICACHE_FLASH_ATTR Camera_stop_pictures();

int ICACHE_FLASH_ATTR Camera_init(struct DeviceConfig *config);
int ICACHE_FLASH_ATTR Camera_set_default_data(struct DeviceConfig *config);

#endif
