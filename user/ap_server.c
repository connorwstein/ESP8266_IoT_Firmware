#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"

#include "helper.h"
#include "device_config.h"
#include "parser.h"
#include "sta_server.h"
#include "wifi.h"

void ICACHE_FLASH_ATTR ap_server_recv_cb(void *arg, char *pdata, unsigned short len)
{
	uint32 remote_ip;
	int remote_port;

	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;
	
	ets_uart_printf("Received %d bytes from %s:%d!\n", len,
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("%s\n", pdata);

	if (len == 0) {
		ets_uart_printf("Error in received message.\n");
		return;
	}
	
	parser_process_data(pdata, arg);
}

void ICACHE_FLASH_ATTR ap_server_sent_cb(void *arg)
{
	uint32 remote_ip;
	int remote_port;

	remote_port=((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip=*(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;
	ets_uart_printf("Sent data to %s:%d!\n", inet_ntoa(remote_ip), remote_port);
}

void ICACHE_FLASH_ATTR ap_server_connect_cb(void *arg)
{
	uint32 remote_ip;
	int remote_port;

	remote_port=((struct espconn*)arg)->proto.tcp->remote_port;
	remote_ip=*(uint32*)((struct espconn*)arg)->proto.tcp->remote_ip;
	ets_uart_printf("New connection from %s:%d!\n", inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("\n");
}

void ICACHE_FLASH_ATTR ap_server_reconnect_cb(void *arg, sint8 err)
{
	ets_uart_printf("Reconnect: err = %d\n", err);
	ets_uart_printf("\n");
}

void ICACHE_FLASH_ATTR ap_server_disconnect_cb(void *arg)
{
	uint32 remote_ip;
	int remote_port;

	remote_port=((struct espconn*)arg)->proto.tcp->remote_port;
	remote_ip=*(uint32*)((struct espconn*)arg)->proto.tcp->remote_ip;

	ets_uart_printf("%s:%d has disconnected!\n",
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("\n");
}

int ICACHE_FLASH_ATTR ap_server_init()
{
	static struct ip_info info;
	static struct espconn server_conn_ap;
	static esp_tcp server_tcp_ap;

	if (!wifi_get_ip_info(SOFTAP_IF, &info)) {
		ets_uart_printf("Failed to get ip info.\n");
		return -1;
	}

	server_tcp_ap.local_port = 80;
	os_memcpy(server_tcp_ap.local_ip, &(info.ip.addr), 4);

	server_conn_ap.type = ESPCONN_TCP;
	server_conn_ap.proto.tcp = &server_tcp_ap;

	if (espconn_regist_sentcb(&server_conn_ap, ap_server_sent_cb) != 0) {
		ets_uart_printf("Failed to register sent callback.\n");
		return -1;
	}

	if (espconn_regist_recvcb(&server_conn_ap, ap_server_recv_cb) != 0) {
		ets_uart_printf("Failed to register recv callback.\n");
		return -1;
	}

	if (espconn_regist_connectcb(&server_conn_ap, ap_server_connect_cb) != 0) {
		ets_uart_printf("Failed to register connect callback.\n");
		return -1;
	}

	if (espconn_regist_reconcb(&server_conn_ap, ap_server_reconnect_cb) != 0) {
		ets_uart_printf("Failed to register reconnect callback.\n");
		return -1;
	}

	if (espconn_regist_disconcb(&server_conn_ap, ap_server_disconnect_cb) != 0) {
		ets_uart_printf("Failed to register disconnect callback.\n");
		return -1;
	}

	server_conn_ap.link_cnt = 0;
	server_conn_ap.reverse = NULL;

	if (espconn_accept(&server_conn_ap) != 0) {
		ets_uart_printf("Failed to accept.\n");
		return -1;
	}

	if (espconn_regist_time(&server_conn_ap, 0, 0) != 0) {
		ets_uart_printf("Failed to set timeout interval.\n");
		return -1;
	}

	ets_uart_printf("Successfully initialized access point TCP server.\n\n");
	return 0;
}
