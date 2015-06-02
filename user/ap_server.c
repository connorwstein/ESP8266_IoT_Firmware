#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "user_config.h"

#include "helper.h"
#include "sta_server.h"

extern bool HAS_RECEIVED_CONNECT_INSTRUCTION;

void ICACHE_FLASH_ATTR connect_to_network(char *pdata, void *arg)
{
	char *ssid;
	char *password;

	ssid = pdata;
	password = separate(pdata, ';');

	if (password == NULL)
		return;

	strip_newline(password);

	if (password) {
		ets_uart_printf("Received command to connect to SSID: %s with password %s\n",
				ssid, password);
	
		if (espconn_disconnect((struct espconn *)arg) != 0)
			ets_uart_printf("Failed to disconnect.\n");

		HAS_RECEIVED_CONNECT_INSTRUCTION = true;
		start_station(ssid, password);
	}

	ets_uart_printf("\n");
}

void ICACHE_FLASH_ATTR set_device_name(char *pdata)
{
	struct DeviceConfig conf;
	uint32 min_len;

	strip_newline(pdata);
	min_len = (strlen(pdata) < sizeof conf.device_name ? strlen(pdata) : sizeof conf.device_name);
	
	os_memset(conf.device_name, 0, sizeof conf.device_name);

	if (is_flash_used()) {
		if (read_device_config(&conf) != 0)
			ets_uart_printf("Failed to read device config.\n");
	}
	
	os_memset(conf.device_name, 0, sizeof conf.device_name);
	os_memcpy(conf.device_name, pdata, min_len);
	ets_uart_printf("Received command to set my device name to %s!\n", conf.device_name);

	if (save_device_config(&conf) != 0)
		ets_uart_printf("Failed to set my name.\n");
	else
		ets_uart_printf("Successfully saved my name!\n");

	ets_uart_printf("\n");
}

void ICACHE_FLASH_ATTR set_device_type(char *pdata)
{
	struct DeviceConfig conf;
	uint32 min_len;

	strip_newline(pdata);
	min_len = (strlen(pdata) < sizeof conf.device_type ? strlen(pdata) : sizeof conf.device_type);
	
	os_memset(&conf, 0, sizeof conf);

	if (is_flash_used()) {
		if (read_device_config(&conf) != 0)
			ets_uart_printf("Failed to read device config.\n");
	}
	
	os_memset(conf.device_type, 0, sizeof conf.device_type);
	os_memcpy(conf.device_type, pdata, min_len);
	ets_uart_printf("Received command to set my device type to %s!\n", conf.device_type);

	if (save_device_config(&conf) != 0)
		ets_uart_printf("Failed to set my type.\n");
	else
		ets_uart_printf("Successfully saved my type!\n");

	ets_uart_printf("\n");
}

void ICACHE_FLASH_ATTR ap_server_recv_cb(void *arg, char *pdata, unsigned short len)
{
	uint32 remote_ip;
	int remote_port;

	remote_port = ((struct espconn*)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32*)((struct espconn*)arg)->proto.tcp->remote_ip;
	
	ets_uart_printf("Received %d bytes from %s:%d!\n", len,
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("%s\n", pdata);

	if (len == 0) {
		ets_uart_printf("Error in received message.\n");
		return;
	}
	
	if (strncmp(pdata, "Connect:", strlen("Connect:")) == 0)
		connect_to_network(pdata + strlen("Connect:"), arg);
	else if (strncmp(pdata, "Name:", strlen("Name:")) == 0)
		set_device_name(pdata + strlen("Name:"));
	else if (strncmp(pdata, "Type:", strlen("Type:")) == 0)
		set_device_type(pdata + strlen("Type:"));
}

void ICACHE_FLASH_ATTR ap_server_sent_cb(void *arg)
{
	uint32 remote_ip;
	int remote_port;

	remote_port=((struct espconn*)arg)->proto.tcp->remote_port;
	remote_ip=*(uint32*)((struct espconn*)arg)->proto.tcp->remote_ip;
	ets_uart_printf("Sent data to %s:%d!\n", inet_ntoa(remote_ip), remote_port);
}

void ICACHE_FLASH_ATTR ap_server_connect_cb(void *arg)
{
	uint32 remote_ip;
	int remote_port;

	remote_port=((struct espconn*)arg)->proto.tcp->remote_port;
	remote_ip=*(uint32*)((struct espconn*)arg)->proto.tcp->remote_ip;
	ets_uart_printf("New connection from %s:%d!\n",inet_ntoa(remote_ip), remote_port);
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
