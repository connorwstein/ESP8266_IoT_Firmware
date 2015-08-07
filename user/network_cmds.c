#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"

#include "user_config.h"
#include "device_config.h"
#include "helper.h"
#include "wifi.h"

#include "debug.h"

/* Update this if we ever use types longer than 70 chars! */
#define BROADCAST_RESPONSE_BUF_SIZE 200

extern void ICACHE_FLASH_ATTR init_done();

void ICACHE_FLASH_ATTR go_back_to_ap()
{
	DEBUG("enter go_back_to_ap");
	static ETSTimer init_done_timer;
	char ssid[32];
	char password[64] = DEFAULT_AP_PASSWORD;
	uint8 channel = DEFAULT_AP_CHANNEL;
	uint8 mode;

	mode = wifi_get_opmode();

	if (mode == SOFTAP_MODE) {
		ets_uart_printf("Already in AP mode.\n");
		DEBUG("exit go_back_to_ap");
		return;
	} else if (mode != STATION_MODE && mode != STATIONAP_MODE) {
		ets_uart_printf("Was in an invalid opmode. Switching back to AP mode.\n");
		wifi_set_opmode(SOFTAP_MODE);

		generate_default_ssid(ssid, sizeof ssid);

		set_connected_as_station(false);
		set_received_connect_instruction(false);

		if (start_access_point(ssid, password, channel) != 0) {
			ets_uart_printf("Failed to start access point.\n");
			DEBUG("exit go_back_to_ap");
			return;
		}

		init_done();
		DEBUG("exit go_back_to_ap");
		return;
	}

	sta_server_close();
	generate_default_ssid(ssid, sizeof ssid);

	set_connected_as_station(false);
	set_received_connect_instruction(false);

	if (!wifi_station_disconnect())
		ets_uart_printf("Failed to disconnect as station.\n");

	wifi_set_opmode(SOFTAP_MODE);

	if (start_access_point(ssid, password, channel) != 0) {
		ets_uart_printf("Failed to start access point.\n");
		DEBUG("exit go_back_to_ap");
		return;
	}

	/* Need to exit the function to make sure the ip_info gets set.
	   Call init_done in the timer callback instead. */
	os_timer_disarm(&init_done_timer);
	os_timer_setfn(&init_done_timer, init_done, NULL);
	os_timer_arm(&init_done_timer, 1, false);
	DEBUG("exit go_back_to_ap");
}

void ICACHE_FLASH_ATTR connect_to_network(const char *ssid, const char *password)
{
	DEBUG("enter connect_to_network");
	ets_uart_printf("Received command to connect to SSID: %s with password %s\n", ssid, password);
	
	ap_server_close();
	ets_uart_printf("Disconnected\n");

	set_received_connect_instruction(true);
	start_station(ssid, password);
	ets_uart_printf("\n");
	DEBUG("exit connect_to_network");
}

void ICACHE_FLASH_ATTR udp_send_deviceinfo(struct espconn *conn)
{
	DEBUG("enter udp_send_deviceinfo");
	struct ip_info info;
	struct DeviceConfig conf;
	uint8 *ipaddress;
	uint8 mac[6];
	char buff[BROADCAST_RESPONSE_BUF_SIZE];

	if (!wifi_get_ip_info(STATION_IF, &info)) {
		ets_uart_printf("Unable to get ip address of station\n");
		DEBUG("exit udp_send_deviceinfo");
		return;
	}

	ipaddress = (uint8 *)inet_ntoa(info.ip.addr);

	/* Use the softap macaddress for the locators ssids. */
	if (!wifi_get_macaddr(SOFTAP_IF, mac)) {
		ets_uart_printf("Failed to get MAC address.\n");
		DEBUG("exit udp_send_deviceinfo");
		return;
	}

	if (DeviceConfig_read_config(&conf) != 0) {
		ets_uart_printf("Failed to read device config.\n");
		DEBUG("exit udp_send_deviceinfo");
		return;
	}

	os_memset(buff, 0, sizeof buff);

	if (os_strlen(conf.name) > 0)
		os_sprintf(buff, "NAME:%s IP:%s MAC:%s ROOM:%s TYPE:%s",
				conf.name, inet_ntoa(info.ip.addr), str_mac(mac),
				conf.room, DeviceConfig_strtype(conf.type));
	else
		os_sprintf(buff, "NAME:%s IP:%s MAC:%s ROOM:%s TYPE:%s",
				str_mac(mac), inet_ntoa(info.ip.addr), str_mac(mac),
				conf.room, DeviceConfig_strtype(conf.type));

	ets_uart_printf("Sending my device info: %s\n", buff);

	if (espconn_sent(conn, (uint8 *)buff, os_strlen(buff)) != 0)
		ets_uart_printf("espconn_sent failed.\n");

	DeviceConfig_delete(&conf);
	DEBUG("exit udp_send_deviceinfo");
}
