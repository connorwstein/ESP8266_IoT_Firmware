#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "mem.h"

#include "ap_server.h"
#include "helper.h"
#include "device_config.h"
#include "parser.h"

#include "debug.h"

#define TCP_MAX_PACKET_SIZE 1460

struct espconn *sta_server_conn = NULL;

static uint8 *large_buffer = NULL;
static uint8 *cur_buffer = NULL;
static uint16 large_buffer_len = 0;

static void ICACHE_FLASH_ATTR sta_tcpserver_recv_cb(void *arg, char *pdata, unsigned short len)
{
	DEBUG("enter sta_tcpserver_recv_cb");
	uint32 remote_ip;
	int remote_port;

	ets_uart_printf("sta_tcpserver_recv_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;

	ets_uart_printf("Received %d bytes from %s:%d!\n", len,
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("%s\n", pdata);
	ets_uart_printf("\n");

	parser_process_data(pdata, arg);
	DEBUG("exit sta_tcserver_recv_cb");
}

static void ICACHE_FLASH_ATTR sta_udpserver_recv_cb(void *arg, char *pdata, unsigned short len)
{
	DEBUG("enter sta_udpserver_recv_cb");
	uint32 remote_ip;
	int remote_port;
	struct DeviceConfig conf;
	char command_buf[sizeof "Hello " + sizeof conf.type + sizeof " Devices?"];

	ets_uart_printf("sta_udpserver_recv_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;

	ets_uart_printf("Received %d bytes from %s:%d!\n", len,
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("%s\n", pdata);
	ets_uart_printf("\n");

	parser_process_data(pdata, arg);
	DEBUG("exit sta_udpserver_recv_cb");
}

static void ICACHE_FLASH_ATTR sta_tcpserver_sent_cb(void *arg)
{
	DEBUG("enter sta_tcpserver_sent_cb");
	uint32 remote_ip;
	int remote_port;
	int rc;

	ets_uart_printf("sta_tcpserver_sent_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;
	ets_uart_printf("Sent data to %s:%d!\n",
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("\n");

	if (large_buffer != NULL) {
		ets_uart_printf("More data to send.\n");
		large_buffer_len -= TCP_MAX_PACKET_SIZE;
		cur_buffer += TCP_MAX_PACKET_SIZE;

		if (large_buffer_len <= TCP_MAX_PACKET_SIZE) {
			rc = espconn_sent((struct espconn *)arg, cur_buffer, large_buffer_len);

			if (rc != ESPCONN_OK)
				ets_uart_printf("espconn_sent failed: %d\n", rc);

			os_free(large_buffer);
			large_buffer = NULL;
			cur_buffer = NULL;
			large_buffer_len = 0;
		} else {
			rc = espconn_sent((struct espconn *)arg, cur_buffer, TCP_MAX_PACKET_SIZE);

			if (rc != ESPCONN_OK)
				ets_uart_printf("espconn_sent failed: %d\n", rc);
		}
	}

	DEBUG("exit sta_tcpserver_sent_cb");
}

static void ICACHE_FLASH_ATTR sta_udpserver_sent_cb(void *arg)
{
	DEBUG("enter sta_udpserver_sent_cb");
	uint32 remote_ip;
	int remote_port;

	ets_uart_printf("sta_tcpserver_sent_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;
	ets_uart_printf("Sent data to %s:%d!\n",
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("\n");
	DEBUG("exit sta_udpserver_sent_cb");
}

static void ICACHE_FLASH_ATTR sta_tcpserver_connect_cb(void *arg)
{
	DEBUG("enter sta_tcpserver_connect_cb");
	uint32 remote_ip;
	int remote_port;

	ets_uart_printf("sta_tcpserver_connect_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;
	ets_uart_printf("New connection from %s:%d!\n",
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("\n");
	DEBUG("exit sta_tcpserver_connect_cb");
}

static void ICACHE_FLASH_ATTR sta_tcpserver_reconnect_cb(void *arg, sint8 err)
{
	DEBUG("enter sta_tcpserver_reconnect_cb");
	ets_uart_printf("Reconnect: err = %d\n", err);
	ets_uart_printf("\n");
	DEBUG("exit sta_tcpserver_reconnect_cb");
}

static void ICACHE_FLASH_ATTR sta_tcpserver_disconnect_cb(void *arg)
{
	DEBUG("enter sta_tcpserver_disconnect_cb");
	uint32 remote_ip;
	int remote_port;

	ets_uart_printf("sta_tcpserver_disconnect_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;

	ets_uart_printf("%s:%d has disconnected!\n",
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("\n");
	DEBUG("exit sta_tcpserver_disconnect_cb");
}

int ICACHE_FLASH_ATTR sta_server_init_tcp()
{
	DEBUG("enter sta_server_init_tcp");
	static struct ip_info info;
	static struct espconn server_conn_sta;
	static esp_tcp server_tcp_sta;
	int rc;

	if (!wifi_get_ip_info(STATION_IF, &info)) {
		ets_uart_printf("Failed to get ip info.\n");
		DEBUG("exit sta_server_init_tcp");
		return -1;
	}

	server_tcp_sta.local_port = 80;
	os_memcpy(server_tcp_sta.local_ip, &(info.ip.addr), 4);

	server_conn_sta.type = ESPCONN_TCP;
	server_conn_sta.proto.tcp = &server_tcp_sta;

	if (espconn_regist_sentcb(&server_conn_sta, sta_tcpserver_sent_cb) != 0) {
		ets_uart_printf("Failed to register sent callback.\n");
		DEBUG("exit sta_server_init_tcp");
		return -1;
	}

	if (espconn_regist_recvcb(&server_conn_sta, sta_tcpserver_recv_cb) != 0) {
		ets_uart_printf("Failed to register recv callback.\n");
		DEBUG("exit sta_server_init_tcp");
		return -1;
	}

	if (espconn_regist_connectcb(&server_conn_sta, sta_tcpserver_connect_cb) != 0) {
		ets_uart_printf("Failed to register connect callback.\n");
		DEBUG("exit sta_server_init_tcp");
		return -1;
	}

	if (espconn_regist_reconcb(&server_conn_sta, sta_tcpserver_reconnect_cb) != 0) {
		ets_uart_printf("Failed to register reconnect callback.\n");
		DEBUG("exit sta_server_init_tcp");
		return -1;
	}

	if (espconn_regist_disconcb(&server_conn_sta, sta_tcpserver_disconnect_cb) != 0) {
		ets_uart_printf("Failed to register disconnect callback.\n");
		DEBUG("exit sta_server_init_tcp");
		return -1;
	}

	server_conn_sta.link_cnt = 0;
	server_conn_sta.reverse = NULL;
	
	espconn_disconnect(&server_conn_sta);

	if ((rc = espconn_accept(&server_conn_sta)) != 0) {
		if (rc == ESPCONN_ISCONN) {
			ets_uart_printf("Station mode TCP server already connected.\n");
		} else {
			ets_uart_printf("Failed to accept. %d\n", rc);
			DEBUG("exit sta_server_init_tcp");
			return -1;
		}
	} else {
		/* Set to 0 for unlimited TCP connection time (no timeout) */
		if (espconn_regist_time(&server_conn_sta, 0, 0) != 0) {
			ets_uart_printf("Failed to set timeout interval.\n");
			DEBUG("exit sta_server_init_tcp");
			return -1;
		}
	}

	sta_server_conn = &server_conn_sta;
	ets_uart_printf("Successfully initialized station mode TCP server.\n\n");
	DEBUG("exit sta_server_init_tcp");
	return 0;
}

int ICACHE_FLASH_ATTR sta_server_init_udp()
{
	DEBUG("enter sta_server_init_udp");
	static struct ip_info info;
	static struct espconn server_conn_sta;
	static esp_udp server_udp_sta;
	int rc;

	if (!wifi_get_ip_info(STATION_IF, &info)) {
		ets_uart_printf("Failed to get ip info.\n");
		DEBUG("exit sta_server_init_udp");
		return -1;
	}

	server_udp_sta.local_port = 1025;
	os_memcpy(server_udp_sta.local_ip, &(info.ip.addr), 4);

	server_conn_sta.type = ESPCONN_UDP;
	server_conn_sta.proto.udp = &server_udp_sta;

	server_conn_sta.link_cnt = 0;
	server_conn_sta.reverse = NULL;

	if (espconn_regist_sentcb(&server_conn_sta, sta_udpserver_sent_cb) != 0) {
		ets_uart_printf("Failed to register sent callback.\n");
		DEBUG("exit sta_server_init_udp");
		return -1;
	}

	if (espconn_regist_recvcb(&server_conn_sta, sta_udpserver_recv_cb) != 0) {
		ets_uart_printf("Failed to register recv callback.\n");
		DEBUG("exit sta_server_init_udp");
		return -1;
	}

	if ((rc = espconn_create(&server_conn_sta)) != 0) {
		if (rc == ESPCONN_ISCONN) {
			ets_uart_printf("Station mode UDP server already connected.\n");
		} else {
			ets_uart_printf("Failed to create udp server.\n");
			DEBUG("exit sta_server_init_udp");
			return -1;
		}
	}

	ets_uart_printf("Successfully initialized station mode UDP server.\n\n");
	DEBUG("exit sta_server_init_udp");
	return 0;
}

void sta_server_send_large_buffer(struct espconn *conn, uint8 *buf, uint16 len)
{
	int rc;

	if (len <= TCP_MAX_PACKET_SIZE) {
		rc = espconn_sent(conn, buf, len);

		if (rc != ESPCONN_OK)
			ets_uart_printf("espconn_sent failed: %d\n", rc);

		os_free(buf);
	} else {
		large_buffer = buf;
		cur_buffer = large_buffer;
		large_buffer_len = len;

		rc = espconn_sent(conn, buf, TCP_MAX_PACKET_SIZE);

		if (rc != ESPCONN_OK)
			ets_uart_printf("espconn_sent failed: %d\n", rc);
	}
}
