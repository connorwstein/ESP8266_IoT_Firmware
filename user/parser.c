#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "mem.h"

#include "helper.h"
#include "network_cmds.h"
#include "device_config.h"

#include "temperature.h"
#include "lighting.h"
#include "camera.h"

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

	if (os_strcmp(cmd, "Hello ESP Devices?") == 0) {
		udp_send_deviceinfo((struct espconn *)arg);
		DEBUG("exit parser_process_data");
		return;
	} else if (os_strcmp(cmd, "Connect") == 0) {
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
	} else if (os_strcmp(cmd, "Room") == 0) {
		if (DeviceConfig_set_room(params) != 0)
			send_reply("Failed", (struct espconn *)arg);
		else
			send_reply("Room Set", (struct espconn *)arg);

		DEBUG("exit parser_process_data");
		return;
	} else if (os_strcmp(cmd, "Type") == 0) {
		if (os_strcmp(params, "Temperature") == 0)
			rc = DeviceConfig_set_type(TEMPERATURE);
		else if (os_strcmp(params, "Thermostat") == 0)
			rc = DeviceConfig_set_type(THERMOSTAT);
		else if (os_strcmp(params, "Lighting") == 0)
			rc = DeviceConfig_set_type(LIGHTING);
		else if (os_strcmp(params, "Camera") == 0)
			rc = DeviceConfig_set_type(CAMERA);

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

			break;
		case THERMOSTAT:
			break;
		case LIGHTING:
			if (os_strcmp(cmd, "Lighting Get") == 0)
				Lighting_get_light((struct espconn *)arg);
			else if (os_strcmp(cmd, "Lighting Set") == 0)
				Lighting_toggle_light();

			break;
		case CAMERA:
			if (os_strcmp(cmd, "Camera Take Picture") == 0) {
				if (Camera_take_picture() == 0) {
					send_reply("Picture Taken", (struct espconn *)arg);
				} else {
					ets_uart_printf("Failed to take picture.\n");
					send_reply("Picture Take Fail", (struct espconn *)arg);
				}
			} else if (os_strcmp(cmd, "Camera Get Size") == 0) {
				uint16 size;
				char reply[10];

				if (Camera_read_size(&size) == 0) {
					ets_uart_printf("Got picture size: %d bytes\n", size);
					os_sprintf(reply, "%hu", size + 10);	/* 10 is the response header length */
					send_reply(reply, (struct espconn *)arg);
				} else {
					ets_uart_printf("Failed to read picture size.\n");
					send_reply("Get Size Fail", (struct espconn *)arg);
				}
			} else if (os_strcmp(cmd, "Camera Get Picture") == 0) {
				uint16 size;

				if (Camera_read_size(&size) == 0) {
					ets_uart_printf("Got picture size: %d bytes\n", size);
				} else {
					ets_uart_printf("Failed to read picture size.\n");
					send_reply("Picture Got Fail", (struct espconn *)arg);
				}

				if (Camera_read_content(0x0000, size, 0x000a, (struct espconn *)arg) == 0) {
//					send_reply("Picture Got", (struct espconn *)arg);
				} else {
					ets_uart_printf("Failed to read picture contents.\n");
					send_reply("Picture Got Fail", (struct espconn *)arg);
				}
			} else if (os_strcmp(cmd, "Camera Stop Picture") == 0) {
				if (Camera_stop_pictures() != 0)
					ets_uart_printf("Failed to stop taking pictures.\n");
			}

			break;
		default:
			break;
	}

	DeviceConfig_delete(&config);
	DEBUG("exit parser_process_data");
}
