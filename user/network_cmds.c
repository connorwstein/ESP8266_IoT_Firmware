#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"

#include "device_config.h"
#include "helper.h"
#include "wifi.h"

#define BROADCAST_RESPONSE_BUF_SIZE 100

extern bool HAS_RECEIVED_CONNECT_INSTRUCTION;

void ICACHE_FLASH_ATTR connect_to_network(const char *ssid, const char *password, struct espconn *conn)
{
	ets_uart_printf("Received command to connect to SSID: %s with password %s\n",
				ssid, password);
	
	if (espconn_disconnect(conn) != 0)
		ets_uart_printf("Failed to disconnect.\n");

	ets_uart_printf("Disconnected\n");
	HAS_RECEIVED_CONNECT_INSTRUCTION = true;
	start_station(ssid, password);
	ets_uart_printf("\n");
}

void ICACHE_FLASH_ATTR udp_send_ipmac(struct espconn *conn)
{
	struct ip_info info;
	struct DeviceConfig conf;
	uint8 *ipaddress;
	uint8 mac[6];
	char buff[BROADCAST_RESPONSE_BUF_SIZE];

	if (!wifi_get_ip_info(STATION_IF, &info)) {
		ets_uart_printf("Unable to get ip address of station\n");
		return;
	}

	ipaddress = (uint8 *)inet_ntoa(info.ip.addr);

	if (!wifi_get_macaddr(STATION_IF, mac)) {
		ets_uart_printf("Failed to get MAC address.\n");
		return;
	}

	if (DeviceConfig_read_config(&conf) != 0) {
		ets_uart_printf("Failed to read device config.\n");
		return;
	}

	os_memset(buff, 0, sizeof buff);

	if (strlen(conf.name) > 0)
		os_sprintf(buff, "NAME:%s IP:%s MAC:%s", conf.name, inet_ntoa(info.ip.addr), str_mac(mac));
	else
		os_sprintf(buff, "NAME:%s IP:%s MAC:%s", str_mac(mac), inet_ntoa(info.ip.addr), str_mac(mac));

	ets_uart_printf("Sending my NAME, IP and MAC address: %s\n", buff);

	if (espconn_sent(conn, (uint8 *)buff, os_strlen(buff)) != 0)
		ets_uart_printf("espconn_sent failed.\n");

	DeviceConfig_delete(&conf);
}
