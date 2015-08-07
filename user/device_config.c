#include "c_types.h"
#include "osapi.h"
#include "mem.h"

#include "flash.h"
#include "device_config.h"

#include "helper.h"
#include "debug.h"

#include "camera.h"
#include "lighting.h"
#include "temperature.h"

const char *ICACHE_FLASH_ATTR DeviceConfig_strtype(enum DeviceType_t type)
{
	switch (type) {
		case NONE:		return "None";
		case TEMPERATURE:	return "Temperature";
		case HEATER:		return "Heater";
		case LIGHTING:		return "Lighting";
		case CAMERA:		return "Camera";

		default:		return "???";
	}
}

void ICACHE_FLASH_ATTR DeviceConfig_delete(struct DeviceConfig *conf)
{
	DEBUG("enter DeviceConfig_delete");
	os_free(conf->data);
	DEBUG("exit DeviceConfig_delete");
}

int ICACHE_FLASH_ATTR DeviceConfig_already_exists()
{
	DEBUG("enter DeviceConfig_already_exists");
	uint32 guard = 0;

	if (read_from_flash(DEVICE_CONFIG_GUARD_ADDR, &guard, sizeof guard) != 0) {
		DEBUG("exit DeviceConfig_already_exists");
		return 0;
	}

	if (guard == DEVICE_CONFIG_GUARD_VALUE) {
		DEBUG("exit DeviceConfig_already_exists");
		return 1;
	}

	DEBUG("exit DeviceConfig_already_exists");
	return 0;
}

int ICACHE_FLASH_ATTR DeviceConfig_save_config(const struct DeviceConfig *conf)
{
	DEBUG("enter DeviceConfig_save_config");
	const uint32 guard = DEVICE_CONFIG_GUARD_VALUE;
	char *buf;

	buf = (char *)os_zalloc(sizeof guard + DEVICE_CONFIG_FIXED_LEN + FLASH_ALIGN_LEN(conf->data_len));

	if (buf == NULL) {
		DEBUG("exit DeviceConfig_save_config");
		return -1;
	}

	os_memcpy(buf, &guard, sizeof guard);
	os_memcpy(buf + sizeof guard + DEVICE_CONFIG_NAME_OFFSET, conf->name, sizeof conf->name);
	os_memcpy(buf + sizeof guard + DEVICE_CONFIG_ROOM_OFFSET, conf->room, sizeof conf->room);
	os_memcpy(buf + sizeof guard + DEVICE_CONFIG_TYPE_OFFSET, &(conf->type), sizeof conf->type);
	os_memcpy(buf + sizeof guard + DEVICE_CONFIG_LEN_OFFSET, &(conf->data_len), sizeof conf->data_len);

	if (conf->data != NULL && conf->data_len != 0) {
		os_memcpy(buf + sizeof guard + DEVICE_CONFIG_DATA_OFFSET, conf->data, conf->data_len);
	}

	if (write_to_flash(DEVICE_CONFIG_GUARD_ADDR, (uint32 *)buf,
				sizeof guard + DEVICE_CONFIG_FIXED_LEN + FLASH_ALIGN_LEN(conf->data_len)) != 0) {
		os_free(buf);
		DEBUG("exit DeviceConfig_save_config");
		return -1;
	}

	os_free(buf);
	DEBUG("exit DeviceConfig_save_config");
	return 0;
}

int ICACHE_FLASH_ATTR DeviceConfig_read_config(struct DeviceConfig *conf)
{
	DEBUG("enter DeviceConfig_read_config");
	char name[sizeof conf->name];
	char room[sizeof conf->room];
	enum DeviceType_t type = NONE;
	uint32 data_len = 0;
	void *data = NULL;

	os_memset(name, 0, sizeof name);
	os_memset(room, 0, sizeof room);
	os_memset(conf, 0, sizeof *conf);

	if (read_from_flash(DEVICE_CONFIG_NAME_ADDR, (uint32 *)name, sizeof name) != 0) {
		DEBUG("exit DeviceConfig_read_config");
		return -1;
	}

	if (read_from_flash(DEVICE_CONFIG_ROOM_ADDR, (uint32 *)room, sizeof room) != 0) {
		DEBUG("exit DeviceConfig_read_config");
		return -1;
	}

	if (read_from_flash(DEVICE_CONFIG_TYPE_ADDR, (uint32 *)&type, sizeof type) != 0) {
		DEBUG("exit DeviceConfig_read_config");
		return -1;
	}

	if (read_from_flash(DEVICE_CONFIG_LEN_ADDR, &data_len, sizeof data_len) != 0) {
		DEBUG("exit DeviceConfig_read_config");
		return -1;
	}

	if (data_len == 0) {
		data = NULL;
	} else {
		data = (char *)os_zalloc(FLASH_ALIGN_LEN(data_len));

		if (data == NULL) {
			DEBUG("exit DeviceConfig_read_config");
			return -1;
		}

		if (read_from_flash(DEVICE_CONFIG_DATA_ADDR, data, FLASH_ALIGN_LEN(data_len)) != 0) {
			os_free(data);
			DEBUG("exit DeviceConfig_read_config");
			return -1;
		}
	}

	os_memcpy(conf->name, name, sizeof conf->name);
	os_memcpy(conf->room, room, sizeof conf->room);
	conf->type = type;
	conf->data_len = data_len;
	conf->data = data;
	DEBUG("exit DeviceConfig_read_config");
	return 0;
}

int ICACHE_FLASH_ATTR DeviceConfig_set_name(const char *name)
{
	DEBUG("enter DeviceConfig_set_name");
	struct DeviceConfig conf;
	uint32 min_len;

	ets_uart_printf("set_device_name: %s\n", name);

	/* Leave space for NULL char */
	min_len = (strlen(name) < sizeof conf.name - 1 ? strlen(name) : sizeof conf.name - 1);
	os_memset(&conf, 0, sizeof conf);

	if (DeviceConfig_already_exists()) {
		if (DeviceConfig_read_config(&conf) != 0)
			ets_uart_printf("Failed to read device config. Resetting config.\n");
	}

	os_memset(conf.name, 0, sizeof conf.name);
	os_memcpy(conf.name, name, min_len);
	ets_uart_printf("Will set my device name to %s!\n", conf.name);

	if (DeviceConfig_save_config(&conf) != 0) {
		ets_uart_printf("Failed to save config.\n");
		DEBUG("exit DeviceConfig_set_name");
		return -1;
	}

	os_free(conf.data);
	ets_uart_printf("Successfully saved new config!\n");
	ets_uart_printf("\n");
	DEBUG("exit DeviceConfig_set_name");
	return 0;
}

int ICACHE_FLASH_ATTR DeviceConfig_set_room(const char *room)
{
	DEBUG("enter DeviceConfig_set_room");
	struct DeviceConfig conf;
	uint32 min_len;

	ets_uart_printf("set_device_room: %s\n", room);

	/* Leave space for NULL char */
	min_len = (strlen(room) < sizeof conf.room - 1 ? strlen(room) : sizeof conf.room - 1);
	os_memset(&conf, 0, sizeof conf);

	if (DeviceConfig_already_exists()) {
		if (DeviceConfig_read_config(&conf) != 0)
			ets_uart_printf("Failed to read device config. Resetting config.\n");
	}

	os_memset(conf.room, 0, sizeof conf.room);
	os_memcpy(conf.room, room, min_len);
	ets_uart_printf("Will set my device room to %s!\n", conf.room);

	if (DeviceConfig_save_config(&conf) != 0) {
		ets_uart_printf("Failed to save config.\n");
		DEBUG("exit DeviceConfig_set_name");
		return -1;
	}

	os_free(conf.data);
	ets_uart_printf("Successfully saved new config!\n");
	ets_uart_printf("\n");
	DEBUG("exit DeviceConfig_set_name");
	return 0;
}

static int device_set_default_data(struct DeviceConfig *config, enum DeviceType_t type)
{
	switch (type) {
		case TEMPERATURE:
				return Temperature_set_default_data(config);

		case HEATER:
				return Heater_set_default_data(config);

		case LIGHTING:
				return Lighting_set_default_data(config);

		case CAMERA:
				return Camera_set_default_data(config);

		default:
			break;
	}

	return 0;
}

int ICACHE_FLASH_ATTR DeviceConfig_set_type(enum DeviceType_t type)
{
	DEBUG("enter DeviceConfig_set_type");
	struct DeviceConfig conf;

	os_memset(&conf, 0, sizeof conf);

	if (DeviceConfig_already_exists()) {
		if (DeviceConfig_read_config(&conf) != 0)
			ets_uart_printf("Failed to read device config. Resetting config.\n");
	}

	if (type == conf.type) {
		ets_uart_printf("Device already has this type.\n");
		return 0;
	}

	/* Free any previous data info. */
	if (conf.data)
		os_free(conf.data);

	conf.data = NULL;
	conf.data_len = 0;

	if (device_set_default_data(&conf, type) != 0) {
		ets_uart_printf("Failed to set default data for type %d.\n", type);
		DEBUG("exit DeviceConfig_set_type");
		return -1;
	}

	conf.type = type;
	ets_uart_printf("Will set my device type to %d!\n", conf.type);

	if (DeviceConfig_save_config(&conf) != 0) {
		ets_uart_printf("Failed to save config.\n");
		DEBUG("exit DeviceConfig_set_type");
		return -1;
	}

	/* Free the device config struct data. */
	if (conf.data)
		os_free(conf.data);

	if (!DeviceInit()) {
		ets_uart_printf("Failed to initialize as new device type.\n");
		return -1;
	}

	ets_uart_printf("Successfully saved new config!\n");
	ets_uart_printf("\n");
	DEBUG("exit DeviceConfig_set_type");
	return 0;
}

int ICACHE_FLASH_ATTR DeviceConfig_set_data(const void *data, uint32 len)
{
	DEBUG("enter DeviceConfig_set_data");
	struct DeviceConfig conf;	

	os_memset(&conf, 0, sizeof conf);

	if (DeviceConfig_already_exists()) {
		if (DeviceConfig_read_config(&conf) != 0)
			ets_uart_printf("Failed to read device config. Resetting config.\n");
	}

	conf.data = data;
	conf.data_len = len;

	ets_uart_printf("Will write %d bytes to my data!\n", conf.data_len);

	if (DeviceConfig_save_config(&conf) != 0) {
		ets_uart_printf("Failed to save config.\n");
		DEBUG("exit DeviceConfig_set_data");
		return -1;
	}

	ets_uart_printf("Successfully saved new config!\n");
	ets_uart_printf("\n");
	DEBUG("exit DeviceConfig_set_data");
	return 0;
}

bool ICACHE_FLASH_ATTR DeviceInit()
{
	DEBUG("enter DeviceInit");
	struct DeviceConfig conf;

	if (!DeviceConfig_already_exists())
		return false;

	if (DeviceConfig_read_config(&conf) != 0)
		return false;

	switch (conf.type) {
		case TEMPERATURE:
				if (Temperature_init(&conf) != 0) {
					ets_uart_printf("Failed to init temperature.\n");
					return false;
				}

				break;

		case HEATER:
				if (Heater_init(&conf) != 0) {
					ets_uart_printf("Failed to init heater.\n");
					return false;
				}

				break;

		case LIGHTING:
				if (Lighting_init(&conf) != 0) {
					ets_uart_printf("Failed to init lighting.\n");
					return false;
				}

				break;

		case CAMERA:
				if (Camera_init(&conf) != 0) {
					ets_uart_printf("Failed to init camera.\n");
					return false;
				}

				/* Strange, if we write if (!Camera_reset()) {, the device restarts
				   due to lack of response from camera... Could be some crazy timing issue... */
				if (Camera_reset() != 0) {
					ets_uart_printf("Failed to reset camera.\n");
					return false;
				}

				break;

		default:
				break;
	}

	DEBUG("exit DeviceInit");
	return true;
}
