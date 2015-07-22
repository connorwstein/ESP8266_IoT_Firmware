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

extern struct espconn *ap_server_conn;
extern struct espconn *sta_server_conn;

void ICACHE_FLASH_ATTR go_back_to_ap(struct espconn *conn)
{
	DEBUG("enter go_back_to_ap");
	char ssid[32];
	char password[64] = DEFAULT_AP_PASSWORD;
	uint8 channel = DEFAULT_AP_CHANNEL;

	if (conn != NULL) {
		if (espconn_disconnect(conn) != 0)
			ets_uart_printf("Failed to disconnect station client espconn.\n");
	}

	if (sta_server_conn != NULL) {
//		if (espconn_disconnect(sta_server_conn) != 0)
//			ets_uart_printf("Failed to disconnect station server espconn.\n");
	}

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

	init_done();
	DEBUG("exit go_back_to_ap");
}

void ICACHE_FLASH_ATTR connect_to_network(const char *ssid, const char *password, struct espconn *conn)
{
	DEBUG("enter connect_to_network");
	ets_uart_printf("Received command to connect to SSID: %s with password %s\n",
				ssid, password);
	
	if (espconn_disconnect(conn) != 0)
		ets_uart_printf("Failed to disconnect client espconn.\n");

	if (ap_server_conn != NULL) {
//		if (espconn_disconnect(ap_server_conn) != 0)
//			ets_uart_printf("Failed to disconnect access point espconn.\n");

//		ap_server_conn = NULL;
	}

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

	if (!wifi_get_macaddr(STATION_IF, mac)) {
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

	if (strlen(conf.name) > 0)
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
