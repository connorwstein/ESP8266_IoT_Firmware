#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include "c_types.h"

#define DEVICE_CONFIG_NAME_OFFSET 0
#define DEVICE_CONFIG_ROOM_OFFSET 32
#define DEVICE_CONFIG_TYPE_OFFSET 64
#define DEVICE_CONFIG_LEN_OFFSET  68
#define DEVICE_CONFIG_DATA_OFFSET 72

#define DEVICE_CONFIG_GUARD_ADDR  0x3C000
#define DEVICE_CONFIG_GUARD_VALUE 0xDEADBEEF

#define DEVICE_CONFIG_BASE_ADDR 0x3C004
#define DEVICE_CONFIG_NAME_ADDR (DEVICE_CONFIG_BASE_ADDR + DEVICE_CONFIG_NAME_OFFSET)
#define DEVICE_CONFIG_ROOM_ADDR (DEVICE_CONFIG_BASE_ADDR + DEVICE_CONFIG_ROOM_OFFSET)
#define DEVICE_CONFIG_TYPE_ADDR (DEVICE_CONFIG_BASE_ADDR + DEVICE_CONFIG_TYPE_OFFSET)
#define DEVICE_CONFIG_LEN_ADDR	(DEVICE_CONFIG_BASE_ADDR + DEVICE_CONFIG_LEN_OFFSET)
#define DEVICE_CONFIG_DATA_ADDR	(DEVICE_CONFIG_BASE_ADDR + DEVICE_CONFIG_DATA_OFFSET)

#define DEVICE_CONFIG_FIXED_LEN 72

enum DeviceType_t {
	NONE = 0,
	TEMPERATURE,
	HEATER,
	LIGHTING,
	CAMERA
};

struct DeviceConfig {
	const char name[32];
	const char room[32];
	enum DeviceType_t type;
	uint32 data_len;
	const void *data;
};

const char *ICACHE_FLASH_ATTR DeviceConfig_strtype(enum DeviceType_t type);

void ICACHE_FLASH_ATTR DeviceConfig_delete(struct DeviceConfig *conf);

int ICACHE_FLASH_ATTR DeviceConfig_already_exists();
int ICACHE_FLASH_ATTR DeviceConfig_save_config(const struct DeviceConfig *conf);
int ICACHE_FLASH_ATTR DeviceConfig_read_config(struct DeviceConfig *conf);

int ICACHE_FLASH_ATTR DeviceConfig_set_name(const char *name);
int ICACHE_FLASH_ATTR DeviceCOnfig_set_room(const char *room);
int ICACHE_FLASH_ATTR DeviceConfig_set_type(enum DeviceType_t type);
int ICACHE_FLASH_ATTR DeviceConfig_set_data(const void *data, uint32 len);

bool ICACHE_FLASH_ATTR DeviceInit();

#endif
