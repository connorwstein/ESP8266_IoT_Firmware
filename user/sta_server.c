#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "user_config.h"

#include "helper.h"

extern struct espconn server_conn;
extern esp_tcp server_tcp;

void ICACHE_FLASH_ATTR sta_wifi_handler(System_Event_t *event)
{
	struct ip_info info;

	switch (event->event) {
		case EVENT_STAMODE_CONNECTED:
			ets_uart_printf("Connected to Access Point!\n");
			ets_uart_printf("SSID: %s\n", event->event_info.connected.ssid);
			ets_uart_printf("BSSID: %s\n", str_bssid(event->event_info.connected.bssid));
			ets_uart_printf("Channel: %d\n", event->event_info.connected.channel);
			ets_uart_printf("\n");
			break;

		case EVENT_STAMODE_DISCONNECTED:
			ets_uart_printf("Disconnected from Access Point!\n");
			ets_uart_printf("SSID: %s\n", event->event_info.disconnected.ssid);
			ets_uart_printf("BSSID: %s\n", str_bssid(event->event_info.disconnected.bssid));
			ets_uart_printf("Reason: %d\n", event->event_info.disconnected.reason);
			ets_uart_printf("\n");

			if (espconn_delete(&server_conn) != 0)
				ets_uart_printf("Failed to delete connection.\n");

			break;

		case EVENT_STAMODE_GOT_IP:
			ets_uart_printf("Got IP!\n");
			print_ip_info((struct ip_info *)&(event->event_info.got_ip));
			ets_uart_printf("\n");
			sta_server_init();
			break;

		default:
			ets_uart_printf("Got event: %d\n", event->event);
			break;
	}

}

void ICACHE_FLASH_ATTR sta_server_recv_cb(void *arg, char *pdata, unsigned short len)
{
	uint32 remote_ip;
	int remote_port;

	remote_ip = *(uint32 *)server_tcp.remote_ip;
	remote_port = server_tcp.remote_port;

	ets_uart_printf("Received %d bytes from %s:%d!\n", len,
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("%s\n", pdata);
	ets_uart_printf("\n");
}

void ICACHE_FLASH_ATTR sta_server_sent_cb(void *arg)
{
	uint32 remote_ip;
	int remote_port;

	remote_ip = *(uint32 *)server_tcp.remote_ip;
	remote_port = server_tcp.remote_port;

	ets_uart_printf("Sent data to %s:%d!\n",
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("\n");
}

void ICACHE_FLASH_ATTR sta_server_connect_cb(void *arg)
{
	uint32 remote_ip;
	int remote_port;

	remote_ip = *(uint32 *)server_tcp.remote_ip;
	remote_port = server_tcp.remote_port;

	ets_uart_printf("New connection from %s:%d!\n",
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("\n");
}

void ICACHE_FLASH_ATTR sta_server_reconnect_cb(void *arg, sint8 err)
{
	ets_uart_printf("Reconnect: err = %d\n", err);
	ets_uart_printf("\n");
}

void ICACHE_FLASH_ATTR sta_server_disconnect_cb(void *arg)
{
	uint32 remote_ip;
	int remote_port;

	remote_ip = *(uint32 *)server_tcp.remote_ip;
	remote_port = server_tcp.remote_port;

	ets_uart_printf("%s:%d has disconnected!\n",
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("\n");
}

int ICACHE_FLASH_ATTR sta_server_init()
{
	struct ip_info info;

	if (!wifi_get_ip_info(STATION_IF, &info)) {
		ets_uart_printf("Failed to get ip info.\n");
		return -1;
	}

	server_tcp.local_port = 80;
	os_memcpy(server_tcp.local_ip, &(info.ip.addr), 4);

	server_conn.type = ESPCONN_TCP;
	server_conn.state = ESPCONN_NONE;
	server_conn.proto.tcp = &server_tcp;

	if (espconn_regist_sentcb(&server_conn, sta_server_sent_cb) != 0) {
		ets_uart_printf("Failed to register sent callback.\n");
		return -1;
	}

	if (espconn_regist_recvcb(&server_conn, sta_server_recv_cb) != 0) {
		ets_uart_printf("Failed to register recv callback.\n");
		return -1;
	}

	if (espconn_regist_connectcb(&server_conn, sta_server_connect_cb) != 0) {
		ets_uart_printf("Failed to register connect callback.\n");
		return -1;
	}

	if (espconn_regist_reconcb(&server_conn, sta_server_reconnect_cb) != 0) {
		ets_uart_printf("Failed to register reconnect callback.\n");
		return -1;
	}

	if (espconn_regist_disconcb(&server_conn, sta_server_disconnect_cb) != 0) {
		ets_uart_printf("Failed to register disconnect callback.\n");
		return -1;
	}

	server_conn.link_cnt = 0;
	server_conn.reverse = NULL;
	
	if (espconn_accept(&server_conn) != 0) {
		ets_uart_printf("Failed to accept.\n");
		return -1;
	}

	if (espconn_regist_time(&server_conn, 0, 0) != 0) {
		ets_uart_printf("Failed to set timeout interval.\n");
		return -1;
	}

	ets_uart_printf("Successfully initialized station mode TCP server.\n\n");
	return 0;
}
