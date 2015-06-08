#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"

#include "helper.h"
#include "network_cmds.h"
#include "device_config.h"
#include "temperature.h"

void ICACHE_FLASH_ATTR parser_process_data(char *data, void *arg)
{
	struct DeviceConfig config;
	char *cmd = data;
	char *params = separate(data, ':');

	if (os_strcmp(cmd, "Connect") == 0) {
		char *ssid = params;
		char *password = separate(params, ';');
		connect_to_network(ssid, password, (struct espconn *)arg);
		return;
	} else if (os_strcmp(cmd, "Name") == 0) {
		DeviceConfig_set_name(params);
		return;
	} else if (os_strcmp(cmd, "Type") == 0) {
		if (os_strcmp(params, "Temperature") == 0)
			DeviceConfig_set_type(TEMPERATURE);
		else if (os_strcmp(params, "Thermostat") == 0)
			DeviceConfig_set_type(THERMOSTAT);
		else if (os_strcmp(params, "Lighting") == 0)
			DeviceConfig_set_type(LIGHTING);

		return;
	}

	if (!DeviceConfig_already_exists())
		return;

	if (DeviceConfig_read_config(&config) != 0) {
		ets_uart_printf("Failed to read config.\n");
		return;
	}

	switch (config.type) {
		case TEMPERATURE:
			if (os_strcmp(cmd, "Temperature Set") == 0)
				Temperature_set_temperature(atoi(params));	
			else if (os_strcmp(cmd, "Temperature Get") == 0)
				Temperature_get_temperature((struct espconn *)arg);
			else if (os_strcmp(cmd, "Hello Temperature Devices?") == 0)
				udp_send_ipmac((struct espconn *)arg);
			break;
		case THERMOSTAT:
			if (os_strcmp(cmd, "Hello Thermostat Devices?") == 0)
				udp_send_ipmac((struct espconn *)arg);

			break;
		case LIGHTING:
			if (os_strcmp(cmd, "Hello Lighting Devices?") == 0)
				udp_send_ipmac((struct espconn *)arg);

			break;
		default:
			break;
	}

	DeviceConfig_delete(&config);
}
