#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "user_config.h"

#include "helper.h"
#include "sta_server.h"

extern struct espconn ap_server_conn;
extern esp_tcp ap_server_tcp;

void ICACHE_FLASH_ATTR ap_server_recv_cb(void *arg, char *pdata, unsigned short len)
{
	uint32 remote_ip;
	int remote_port;
	char *ssid;
	char *password;

	remote_ip = *(uint32 *)ap_server_tcp.remote_ip;
	remote_port = ap_server_tcp.remote_port;

	ets_uart_printf("Received %d bytes from %s:%d!\n", len,
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("%s\n", pdata);

	ssid = pdata;
	password = separate(pdata, ';');

	if (password) {
		ets_uart_printf("Received command to connect to SSID: %s with password %s\n",
				ssid, password);
	
//		wifi_set_event_handler_cb(sta_wifi_handler);
//		start_station(ssid, password);
//		sta_server_init();
	}
}

void ICACHE_FLASH_ATTR ap_server_sent_cb(void *arg)
{
	uint32 remote_ip;
	int remote_port;

	remote_ip = *(uint32 *)ap_server_tcp.remote_ip;
	remote_port = ap_server_tcp.remote_port;

	ets_uart_printf("Sent data to %s:%d!\n",
			inet_ntoa(remote_ip), remote_port);
}

void ICACHE_FLASH_ATTR ap_server_connect_cb(void *arg)
{
	uint32 remote_ip;
	int remote_port;

	remote_ip = *(uint32 *)ap_server_tcp.remote_ip;
	remote_port = ap_server_tcp.remote_port;

	ets_uart_printf("New connection from %s:%d!\n",
			inet_ntoa(remote_ip), remote_port);
}

void ICACHE_FLASH_ATTR ap_server_reconnect_cb(void *arg, sint8 err)
{
	ets_uart_printf("Reconnect: err = %d\n", err);
}

void ICACHE_FLASH_ATTR ap_server_disconnect_cb(void *arg)
{
	uint32 remote_ip;
	int remote_port;

	remote_ip = *(uint32 *)ap_server_tcp.remote_ip;
	remote_port = ap_server_tcp.remote_port;

	ets_uart_printf("%s:%d has disconnected!\n",
			inet_ntoa(remote_ip), remote_port);
}

int ICACHE_FLASH_ATTR ap_server_init()
{
	struct ip_info info;

	if (!wifi_get_ip_info(SOFTAP_IF, &info)) {
		ets_uart_printf("Failed to get ip info.\n");
		return -1;
	}

	ap_server_tcp.local_port = 80;
	os_memcpy(ap_server_tcp.local_ip, &(info.ip.addr), 4);

	ap_server_conn.type = ESPCONN_TCP;
	ap_server_conn.state = ESPCONN_NONE;
	ap_server_conn.proto.tcp = &ap_server_tcp;

	if (espconn_regist_sentcb(&ap_server_conn, ap_server_sent_cb) != 0) {
		ets_uart_printf("Failed to register sent callback.\n");
		return -1;
	}

	if (espconn_regist_recvcb(&ap_server_conn, ap_server_recv_cb) != 0) {
		ets_uart_printf("Failed to register recv callback.\n");
		return -1;
	}

	if (espconn_regist_connectcb(&ap_server_conn, ap_server_connect_cb) != 0) {
		ets_uart_printf("Failed to register connect callback.\n");
		return -1;
	}

	if (espconn_regist_reconcb(&ap_server_conn, ap_server_reconnect_cb) != 0) {
		ets_uart_printf("Failed to register reconnect callback.\n");
		return -1;
	}

	if (espconn_regist_disconcb(&ap_server_conn, ap_server_disconnect_cb) != 0) {
		ets_uart_printf("Failed to register disconnect callback.\n");
		return -1;
	}

	if (espconn_regist_time(&ap_server_conn, 7200, 1)) {
		ets_uart_printf("Failed to set timeout interval.\n");
		return -1;
	}

	ap_server_conn.link_cnt = 0;
	ap_server_conn.reverse = NULL;
	
	if (espconn_accept(&ap_server_conn) != 0) {
		ets_uart_printf("Failed to accept.\n");
		return -1;
	}

	ets_uart_printf("Successfully initialized access point TCP server.\n\n");
	return 0;
}
