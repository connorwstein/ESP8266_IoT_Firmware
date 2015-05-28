#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "user_config.h"

#include "helper.h"
#include "ap_server.h"

#define BROADCAST_RESPONSE_BUF_SIZE 45
#define MAC_ADDRESS_BYTES 6

static void ICACHE_FLASH_ATTR sta_tcpserver_recv_cb(void *arg, char *pdata, unsigned short len)
{
	uint32 remote_ip;
	int remote_port;

	ets_uart_printf("sta_tcpserver_recv_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;

	ets_uart_printf("Received %d bytes from %s:%d!\n", len,
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("%s\n", pdata);
	ets_uart_printf("\n");
}

static void ICACHE_FLASH_ATTR sta_udpserver_recv_cb(void *arg, char *pdata, unsigned short len)
{
	uint32 remote_ip;
	int remote_port;
	struct ip_info info;
	uint8 *ipaddress;
	uint8 macaddress[MAC_ADDRESS_BYTES];
	char buff[BROADCAST_RESPONSE_BUF_SIZE];

	ets_uart_printf("sta_udpserver_recv_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;

	ets_uart_printf("Received %d bytes from %s:%d!\n", len,
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("%s\n", pdata);
	ets_uart_printf("\n");

	if (!wifi_get_ip_info(STATION_IF, &info)) {
		ets_uart_printf("Unable to get ip address of station\n");
		return;
	}

	ets_uart_printf("Got IP info\n");
	ipaddress=(uint8 *)inet_ntoa(info.ip.addr);

	if (!wifi_get_macaddr(STATION_IF, macaddress)) {
		ets_uart_printf("Failed to get mac address\n");
		return;
	}

	ets_uart_printf("Got mac address\n");
	ets_uart_printf("IP: %s, MAC: %s, len: %d\n", ipaddress, str_mac(macaddress), strlen((char *)ipaddress));
	os_sprintf(buff,"IP:%s, MAC:%s", inet_ntoa(info.ip.addr), str_mac(macaddress));
	buff[strlen(buff)] = '\0';
	ets_uart_printf("Sending: %s", buff);
	espconn_sent((struct espconn *)arg, (uint8 *)buff, strlen(buff));
}

static void ICACHE_FLASH_ATTR sta_tcpserver_sent_cb(void *arg)
{
	uint32 remote_ip;
	int remote_port;

	ets_uart_printf("sta_tcpserver_sent_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;
	ets_uart_printf("Sent data to %s:%d!\n",
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("\n");
}

static void ICACHE_FLASH_ATTR sta_udpserver_sent_cb(void *arg)
{
	uint32 remote_ip;
	int remote_port;

	ets_uart_printf("sta_tcpserver_sent_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;
	ets_uart_printf("Sent data to %s:%d!\n",
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("\n");
}

static void ICACHE_FLASH_ATTR sta_tcpserver_connect_cb(void *arg)
{
	uint32 remote_ip;
	int remote_port;

	ets_uart_printf("sta_tcpserver_connect_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;
	ets_uart_printf("New connection from %s:%d!\n",
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("\n");
}

static void ICACHE_FLASH_ATTR sta_tcpserver_reconnect_cb(void *arg, sint8 err)
{
	ets_uart_printf("Reconnect: err = %d\n", err);
	ets_uart_printf("\n");
}

static void ICACHE_FLASH_ATTR sta_tcpserver_disconnect_cb(void *arg)
{
	uint32 remote_ip;
	int remote_port;

	ets_uart_printf("sta_tcpserver_disconnect_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;

	ets_uart_printf("%s:%d has disconnected!\n",
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("\n");
}

int ICACHE_FLASH_ATTR sta_server_init_tcp()
{
	static struct ip_info info;
	static struct espconn server_conn_sta;
	static esp_tcp server_tcp_sta;

	if (!wifi_get_ip_info(STATION_IF, &info)) {
		ets_uart_printf("Failed to get ip info.\n");
		return -1;
	}

	server_tcp_sta.local_port = 80;
	os_memcpy(server_tcp_sta.local_ip, &(info.ip.addr), 4);

	server_conn_sta.type = ESPCONN_TCP;
	server_conn_sta.proto.tcp = &server_tcp_sta;

	if (espconn_regist_sentcb(&server_conn_sta, sta_tcpserver_sent_cb) != 0) {
		ets_uart_printf("Failed to register sent callback.\n");
		return -1;
	}

	if (espconn_regist_recvcb(&server_conn_sta, sta_tcpserver_recv_cb) != 0) {
		ets_uart_printf("Failed to register recv callback.\n");
		return -1;
	}

	if (espconn_regist_connectcb(&server_conn_sta, sta_tcpserver_connect_cb) != 0) {
		ets_uart_printf("Failed to register connect callback.\n");
		return -1;
	}

	if (espconn_regist_reconcb(&server_conn_sta, sta_tcpserver_reconnect_cb) != 0) {
		ets_uart_printf("Failed to register reconnect callback.\n");
		return -1;
	}

	if (espconn_regist_disconcb(&server_conn_sta, sta_tcpserver_disconnect_cb) != 0) {
		ets_uart_printf("Failed to register disconnect callback.\n");
		return -1;
	}

	server_conn_sta.link_cnt = 0;
	server_conn_sta.reverse = NULL;
	
	if (espconn_accept(&server_conn_sta) != 0) {
		ets_uart_printf("Failed to accept.\n");
		return -1;
	}

	if (espconn_regist_time(&server_conn_sta, 0, 0) != 0) {
		ets_uart_printf("Failed to set timeout interval.\n");
		return -1;
	}

	ets_uart_printf("Successfully initialized station mode TCP server.\n\n");
	return 0;
}

int ICACHE_FLASH_ATTR sta_server_init_udp()
{
	static struct ip_info info;
	static struct espconn server_conn_sta;
	static esp_udp server_udp_sta;

	if (!wifi_get_ip_info(STATION_IF, &info)) {
		ets_uart_printf("Failed to get ip info.\n");
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
		return -1;
	}

	if (espconn_regist_recvcb(&server_conn_sta, sta_udpserver_recv_cb) != 0) {
		ets_uart_printf("Failed to register recv callback.\n");
		return -1;
	}

	if (espconn_create(&server_conn_sta) != 0) {
		ets_uart_printf("Failed to create udp server.\n");
		return -1;
	}

	ets_uart_printf("Successfully initialized station mode UDP server.\n\n");
}
