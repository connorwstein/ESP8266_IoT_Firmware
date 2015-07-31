#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "mem.h"

#include "server.h"

#include "helper.h"
#include "network_cmds.h"
#include "device_config.h"

#include "temperature.h"
#include "lighting.h"
#include "camera.h"

#include "debug.h"

static void ICACHE_FLASH_ATTR send_reply(char *pdata, struct espconn *conn)
{
	char *data;

	data = (char *)os_zalloc(os_strlen(pdata) + 1);

	if (data == NULL) {
		ets_uart_printf("Failed to allocate memory for reply buffer.\n");
		return;
	}

	os_memcpy(data, pdata, os_strlen(pdata));
	ets_uart_printf("Sending reply: %s\n", data);

	if (tcpserver_send(conn, data, os_strlen(data), HEAP_MEM) != 0) {
		ets_uart_printf("Failed to send reply.\n");
		os_free(data);
	}
}

void ICACHE_FLASH_ATTR udpparser_process_data(char *data, uint16 len, struct espconn *conn)
{
	DEBUG("enter udpparser_process_data");
	char *cmd = data;
	char *params = separate(data, ':');

	if (os_strcmp(cmd, "Hello ESP Devices?") == 0) {
#ifdef USE_AS_LOCATOR
		if (in_locator_mode())
			stop_locator_mode();
#endif
		udp_send_deviceinfo(conn);
		DEBUG("exit udpparser_process_data");
		return;
#ifdef USE_AS_LOCATOR
	} else if (os_strcmp(cmd, "Devices Low Power") == 0) {
		/* Need to avoid going to locator mode twice if he sends the commands multiple times! */
		if (!in_locator_mode())
			start_locator_mode();

		udp_send_deviceinfo(conn);
		DEBUG("exit udpparser_process_data");
		return;
	}
#else
	}
#endif

}

void ICACHE_FLASH_ATTR tcpparser_process_data(char *data, uint16 len, struct espconn *conn)
{
	DEBUG("enter tcpparser_process_data");
	struct DeviceConfig config;
	char *cmd = data;
	char *params = separate(data, ':');
	int rc = -1;

	ets_uart_printf("cmd = %s, params = %s\n", cmd, params);

	if (os_strcmp(cmd, "Connect") == 0) {
		char *ssid = params;
		char *password = separate(params, ';');
		connect_to_network(ssid, password);
		DEBUG("exit tcpparser_process_data");
		return;
	} else if (os_strcmp(cmd, "Run AP") == 0) {
		go_back_to_ap();
		DEBUG("exit tcpparser_process_data");
		return;
	} else if (os_strcmp(cmd, "Name") == 0) {
		if (DeviceConfig_set_name(params) != 0)
			send_reply("Failed", conn);
		else
			send_reply("Name Set", conn);

		DEBUG("exit tcpparser_process_data");
		return;
	} else if (os_strcmp(cmd, "Room") == 0) {
		if (DeviceConfig_set_room(params) != 0)
			send_reply("Failed", conn);
		else
			send_reply("Room Set", conn);

		DEBUG("exit tcpparser_process_data");
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
			send_reply("Failed", conn);
		else
			send_reply("Type Set", conn);

		DEBUG("exit tcpparser_process_data");
		return;
	} else if (os_strcmp(cmd, "Mac Get") == 0) {
		uint8 macaddr[6];

		if (!wifi_get_macaddr(SOFTAP_IF, macaddr))
			send_reply("Failed", conn);
		else
			send_reply(str_mac(macaddr), conn);

		DEBUG("exit tcpparser_process_data");
		return;
	}

	if (!DeviceConfig_already_exists()) {
		DEBUG("exit tcpparser_process_data");
		return;
	}

	if (DeviceConfig_read_config(&config) != 0) {
		ets_uart_printf("Failed to read config.\n");
		DEBUG("exit tcpparser_process_data");
		return;
	}

	switch (config.type) {
		case TEMPERATURE:
			if (os_strcmp(cmd, "Temperature Get") == 0)
				Temperature_get_temperature(conn);

			break;
		case THERMOSTAT:
			break;
		case LIGHTING:
			if (os_strcmp(cmd, "Lighting Get") == 0)
				Lighting_get_light(conn);
			else if (os_strcmp(cmd, "Lighting Set") == 0)
				Lighting_toggle_light();

			break;
		case CAMERA:
			ets_intr_lock();

			if (Camera_is_busy()) {
				ets_uart_printf("Camera is currently busy.\n");
				ets_intr_unlock();
				break;
			}

			if (os_strcmp(cmd, "Camera Take Picture") == 0) {
				Camera_set_busy();
				ets_intr_unlock();

				if (Camera_take_picture() == 0) {
					send_reply("Picture Taken", conn);
				} else {
					ets_uart_printf("Failed to take picture.\n");
					send_reply("Picture Take Fail", conn);
				}

				ets_intr_lock();
				Camera_unset_busy();
				ets_intr_unlock();
			} else if (os_strcmp(cmd, "Camera Get Size") == 0) {
				uint16 size;
				char reply[10];

				Camera_set_busy();
				ets_intr_unlock();

				if (Camera_read_size(&size) == 0) {
					ets_uart_printf("Got picture size: %d bytes\n", size);
					os_sprintf(reply, "%hu", size + 10);	/* 10 is the response header length */
					send_reply(reply, conn);
				} else {
					ets_uart_printf("Failed to read picture size.\n");
					send_reply("Get Size Fail", conn);
				}

				ets_intr_lock();
				Camera_unset_busy();
				ets_intr_unlock();
			} else if (os_strcmp(cmd, "Camera Get Picture") == 0) {
				uint16 size;

				Camera_set_busy();
				ets_intr_unlock();

				if (Camera_read_size(&size) == 0) {
					ets_uart_printf("Got picture size: %d bytes\n", size);
				} else {
					ets_uart_printf("Failed to read picture size.\n");
					send_reply("Picture Got Fail", conn);
					ets_intr_lock();
					Camera_unset_busy();
					ets_intr_unlock();
					break;	/* Don't get picture if get size failed. */
				}

				if (!Camera_read_content(0x0000, size, 0x000a, conn) == 0) {
					ets_uart_printf("Failed to read picture contents.\n");
					send_reply("Picture Got Fail", conn);
					ets_intr_lock();
					Camera_unset_busy();
					ets_intr_unlock();
				}

				/* Do not unlock the camera here. It will be unlocked after data was received. */
			} else if (os_strcmp(cmd, "Camera Stop Picture") == 0) {
				Camera_set_busy();
				ets_intr_unlock();

				if (Camera_stop_pictures() != 0)
					ets_uart_printf("Failed to stop taking pictures.\n");

				ets_intr_lock();
				Camera_unset_busy();
				ets_intr_unlock();
			} else if (os_strcmp(cmd, "Camera Compression Ratio") == 0) {
				Camera_set_busy();
				ets_intr_unlock();

				if (Camera_compression_ratio(atoi(params)) == 0)
					send_reply("Camera Compression Ratio Set", conn);
				else
					send_reply("Camera Compression Ratio Fail", conn);

				ets_intr_lock();
				Camera_unset_busy();
				ets_intr_unlock();
			} else {
				ets_intr_unlock();
			}

			break;
		default:
			break;
	}

	ets_uart_printf("DEBUG: heap size = %u\n", system_get_free_heap_size());
	DeviceConfig_delete(&config);
	DEBUG("exit tcpparser_process_data");
}
