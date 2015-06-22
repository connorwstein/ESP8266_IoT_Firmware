#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"

#include "helper.h"
#include "network_cmds.h"
#include "device_config.h"

#include "temperature.h"
#include "lighting.h"

#include "debug.h"

static void ICACHE_FLASH_ATTR send_reply(char *data, struct espconn *conn)
{
	ets_uart_printf("Sending reply: %s\n", data);
	
	if (espconn_sent(conn, (uint8 *)data, os_strlen(data)) != ESPCONN_OK)
		ets_uart_printf("Failed to send reply.\n");
}

void ICACHE_FLASH_ATTR parser_process_data(char *data, void *arg)
{
	DEBUG("enter parser_process_data");
	struct DeviceConfig config;
	char *cmd = data;
	char *params = separate(data, ':');
	int rc = -1;

	if (os_strcmp(cmd, "Connect") == 0) {
		char *ssid = params;
		char *password = separate(params, ';');
		connect_to_network(ssid, password, (struct espconn *)arg);
		DEBUG("exit parser_process_data");
		return;
	} else if (os_strcmp(cmd, "Run AP") == 0) {
		go_back_to_ap((struct espconn *)arg);
		DEBUG("exit parser_process_data");
		return;
	} else if (os_strcmp(cmd, "Name") == 0) {
		if (DeviceConfig_set_name(params) != 0)
			send_reply("Failed", (struct espconn *)arg);
		else
			send_reply("Name Set", (struct espconn *)arg);

		DEBUG("exit parser_process_data");
		return;
	} else if (os_strcmp(cmd, "Type") == 0) {
		if (os_strcmp(params, "Temperature") == 0)
			rc = DeviceConfig_set_type(TEMPERATURE);
		else if (os_strcmp(params, "Thermostat") == 0)
			rc = DeviceConfig_set_type(THERMOSTAT);
		else if (os_strcmp(params, "Lighting") == 0)
			rc = DeviceConfig_set_type(LIGHTING);

		if (rc != 0)
			send_reply("Failed", (struct espconn *)arg);
		else
			send_reply("Type Set", (struct espconn *)arg);

		DEBUG("exit parser_process_data");
		return;
	}

	if (!DeviceConfig_already_exists()) {
		DEBUG("exit parser_process_data");
		return;
	}

	if (DeviceConfig_read_config(&config) != 0) {
		ets_uart_printf("Failed to read config.\n");
		DEBUG("exit parser_process_data");
		return;
	}

	switch (config.type) {
		case TEMPERATURE:
			if (os_strcmp(cmd, "Temperature Get") == 0)
				Temperature_get_temperature((struct espconn *)arg);
			else if (os_strcmp(cmd, "Hello Temperature Devices?") == 0)
				udp_send_ipmac((struct espconn *)arg);
			break;
		case THERMOSTAT:
			if (os_strcmp(cmd, "Hello Thermostat Devices?") == 0)
				udp_send_ipmac((struct espconn *)arg);

			break;
		case LIGHTING:
			if (os_strcmp(cmd, "Lighting Get") == 0)
				Lighting_get_light((struct espconn *)arg);
			else if (os_strcmp(cmd, "Lighting Set") == 0)
				Lighting_toggle_light();
			if (os_strcmp(cmd, "Hello Lighting Devices?") == 0)
				udp_send_ipmac((struct espconn *)arg);

			break;
		default:
			break;
	}

	DeviceConfig_delete(&config);
	DEBUG("exit parser_process_data");
}
