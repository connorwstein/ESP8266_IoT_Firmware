#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "mem.h"

#include "connection_table.h"
#include "device_config.h"
#include "helper.h"
#include "parser.h"

#include "debug.h"

#define SERVER_TASK_PRIO		1
#define SERVER_TASK_QUEUE_LEN		20

#define SERVER_SIG_RX			1
#define SERVER_SIG_TX			2
#define SERVER_SIG_TX_DONE		3

static struct espconn *tcpserver_conn = NULL;
static struct espconn *udpserver_conn = NULL;

static struct ConnectionTable *tcpserver_conn_table = NULL;
static os_event_t server_task_queue[SERVER_TASK_QUEUE_LEN];

static void ICACHE_FLASH_ATTR server_task(os_event_t *e)
{
	DEBUG("enter server_task");
	static bool sending = false;
	void *data;
	uint16 len;
	enum Memtype mem;

	switch (e->sig) {
		case SERVER_SIG_RX:
			ets_intr_lock();

			if (ConnectionTable_recvmsg_unshift(tcpserver_conn_table, (struct espconn *)e->par,
				&data, &len, &mem) == 0) {
				ets_intr_unlock();
				tcpparser_process_data(data, (struct espconn *)e->par);

				if (mem == HEAP_MEM)
					os_free(data);
			} else {
				ets_intr_unlock();
			}

			break;

		case SERVER_SIG_TX:
			ets_intr_lock();

			if (!sending) {
				if (ConnectionTable_sendmsg_unshift(tcpserver_conn_table, (struct espconn *)e->par,
					&data, &len, &mem) == 0) {
					sending = true;
					ets_intr_unlock();

					if (espconn_sent((struct espconn *)e->par, data, len) != ESPCONN_OK)
						ets_uart_printf("Failed to send data.\n\n");

					if (mem == HEAP_MEM)
						os_free(data);
				} else {
					ets_intr_unlock();
				}
			} else {
				ets_intr_unlock();
			}

			break;

		case SERVER_SIG_TX_DONE:
			ets_intr_lock();

			if (ConnectionTable_sendmsg_empty(tcpserver_conn_table, (struct espconn *)e->par)) {
				sending = false;
				ets_intr_unlock();
			} else {
				ets_intr_unlock();
				system_os_post(SERVER_TASK_PRIO, SERVER_SIG_TX, (os_param_t)e->par);
			}

			break;

		default:
			break;
	}

	DEBUG("exit server_task");
}

static void ICACHE_FLASH_ATTR tcpserver_recv_cb(void *arg, char *pdata, unsigned short len)
{
	DEBUG("enter tcpserver_recv_cb");
	uint32 remote_ip;
	int remote_port;
	char *data;

	ets_uart_printf("tcpserver_recv_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;

	ets_uart_printf("Received %d bytes from %s:%d!\n", len,
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("%s\n", pdata);

	if (tcpserver_conn_table == NULL) {
		ets_uart_printf("No connection table. Ignoring event.\n\n");
		DEBUG("exit tcpserver_recv_cb");
		return;
	}

	data = (char *)os_zalloc(len + 1);	/* Leave 1 byte for NULL char */

	if (data == NULL) {
		ets_uart_printf("Failed to alloc memory for recieved data.\n\n");
		DEBUG("exit tcpserver_recv_cb");
		return;
	}

	/* Need to copy it because the pdata might get freed after exiting the callback. */
	os_memcpy(data, pdata, len);

	ets_intr_lock();

	if (ConnectionTable_recvmsg_push(tcpserver_conn_table, (struct espconn *)arg, data, len, HEAP_MEM) != 0) {
		ets_intr_unlock();
		ets_uart_printf("Failed to push recieved message in recv message queue.\n\n");
		return;
	}

	ets_uart_printf("DEBUG: Pushed data to conn table.\n");
	ets_intr_unlock();

	/* Post TCP_SIG_RX to server task. */
	system_os_post(SERVER_TASK_PRIO, SERVER_SIG_RX, (os_param_t)arg);

	ets_uart_printf("\n");
	DEBUG("exit tcpserver_recv_cb");
}

static void ICACHE_FLASH_ATTR udpserver_recv_cb(void *arg, char *pdata, unsigned short len)
{
	DEBUG("enter udpserver_recv_cb");
	uint32 remote_ip;
	int remote_port;
	struct DeviceConfig conf;

	ets_uart_printf("udpserver_recv_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;

	ets_uart_printf("Received %d bytes from %s:%d!\n", len,
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("%s\n", pdata);
	ets_uart_printf("\n");

	udpparser_process_data(pdata, arg);
	DEBUG("exit udpserver_recv_cb");
}

static void ICACHE_FLASH_ATTR tcpserver_sent_cb(void *arg)
{
	DEBUG("enter tcpserver_sent_cb");
	uint32 remote_ip;
	int remote_port;
	int rc;

	ets_uart_printf("tcpserver_sent_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;
	ets_uart_printf("Sent data to %s:%d!\n",
			inet_ntoa(remote_ip), remote_port);

	if (tcpserver_conn_table == NULL) {
		ets_uart_printf("No connection table. Ignoring event.\n\n");
		DEBUG("exit tcpserver_sent_cb");
		return;
	}

	/* Post TCP_SIG_TX_DONE to server task. */
	system_os_post(SERVER_TASK_PRIO, SERVER_SIG_TX_DONE, (os_param_t)arg);

	ets_uart_printf("\n");
	DEBUG("exit tcpserver_sent_cb");
}

static void ICACHE_FLASH_ATTR udpserver_sent_cb(void *arg)
{
	DEBUG("enter udpserver_sent_cb");
	uint32 remote_ip;
	int remote_port;

	ets_uart_printf("tcpserver_sent_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;
	ets_uart_printf("Sent data to %s:%d!\n",
			inet_ntoa(remote_ip), remote_port);
	ets_uart_printf("\n");
	DEBUG("exit udpserver_sent_cb");
}

static void ICACHE_FLASH_ATTR tcpserver_connect_cb(void *arg)
{
	DEBUG("enter tcpserver_connect_cb");
	uint32 remote_ip;
	int remote_port;

	ets_uart_printf("tcpserver_connect_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;
	ets_uart_printf("New connection from %s:%d!\n",
			inet_ntoa(remote_ip), remote_port);

	if (tcpserver_conn_table == NULL) {
		ets_uart_printf("No connection table. Ignoring event.\n\n");
		DEBUG("exit tcpserver_connect_cb");
		return;
	}

	ets_intr_lock();

	if (ConnectionTable_insert(tcpserver_conn_table, (struct espconn *)arg) != 0) {
		ets_intr_unlock();
		ets_uart_printf("Failed to insert connection in connection table.\n");
	} else {
		ets_intr_unlock();
	}

	ets_uart_printf("\n");
	DEBUG("exit tcpserver_connect_cb");
}

static void ICACHE_FLASH_ATTR tcpserver_reconnect_cb(void *arg, sint8 err)
{
	DEBUG("enter tcpserver_reconnect_cb");
	ets_uart_printf("Reconnect: err = %d\n", err);

	if (tcpserver_conn_table == NULL) {
		ets_uart_printf("No connection table. Ignoring event.\n\n");
		DEBUG("exit tcpserver_reconnect_cb");
		return;
	}

	ets_intr_lock();

	if (ConnectionTable_delete(tcpserver_conn_table, (struct espconn *)arg) != 0)
		ets_uart_printf("Failed to delete connection from table.\n");

	if (ConnectionTable_insert(tcpserver_conn_table, (struct espconn *)arg) != 0)
		ets_uart_printf("Failed to re-insert connection in table.\n");

	ets_intr_unlock();
	ets_uart_printf("\n");
	DEBUG("exit tcpserver_reconnect_cb");
}

static void ICACHE_FLASH_ATTR tcpserver_disconnect_cb(void *arg)
{
	DEBUG("enter tcpserver_disconnect_cb");
	uint32 remote_ip;
	int remote_port;

	ets_uart_printf("tcpserver_disconnect_cb\n");
	remote_port = ((struct espconn *)arg)->proto.tcp->remote_port;
	remote_ip = *(uint32 *)((struct espconn *)arg)->proto.tcp->remote_ip;

	ets_uart_printf("%s:%d has disconnected!\n",
			inet_ntoa(remote_ip), remote_port);

	if (tcpserver_conn_table == NULL) {
		ets_uart_printf("No connection table. Ignoring event.\n\n");
		DEBUG("exit tcpserver_disconnect_cb");
		return;
	}

/*	ets_intr_lock();

	if (ConnectionTable_delete(tcpserver_conn_table, (struct espconn *)arg) != 0) {
		ets_intr_unlock();
		ets_uart_printf("Failed to delete connection from table.\n\n");
	} else {
		ets_intr_unlock();
	}
*/
	ets_uart_printf("\n");
	DEBUG("exit tcpserver_disconnect_cb");
}

static int ICACHE_FLASH_ATTR server_init_tcp(uint8 ifnum)
{
	DEBUG("enter server_init_tcp");
	static struct ip_info info;
	static struct espconn server_conn;
	static esp_tcp server_tcp;
	int rc;

	if (!wifi_get_ip_info(ifnum, &info)) {
		ets_uart_printf("Failed to get ip info.\n");
		DEBUG("exit server_init_tcp");
		return -1;
	}

	tcpserver_conn_table = ConnectionTable_create();

	if (tcpserver_conn_table == NULL) {
		ets_uart_printf("Failed to initialize station TCP server connection table.\n");
		DEBUG("exit server_init_tcp");
		return -1;
	}

	system_os_task(server_task, SERVER_TASK_PRIO, server_task_queue, SERVER_TASK_QUEUE_LEN);

	server_tcp.local_port = 80;
	os_memcpy(server_tcp.local_ip, &(info.ip.addr), 4);

	server_conn.type = ESPCONN_TCP;
	server_conn.proto.tcp = &server_tcp;

	if (espconn_regist_sentcb(&server_conn, tcpserver_sent_cb) != 0) {
		ets_uart_printf("Failed to register sent callback.\n");
		DEBUG("exit server_init_tcp");
		return -1;
	}

	if (espconn_regist_recvcb(&server_conn, tcpserver_recv_cb) != 0) {
		ets_uart_printf("Failed to register recv callback.\n");
		DEBUG("exit server_init_tcp");
		return -1;
	}

	if (espconn_regist_connectcb(&server_conn, tcpserver_connect_cb) != 0) {
		ets_uart_printf("Failed to register connect callback.\n");
		DEBUG("exit server_init_tcp");
		return -1;
	}

	if (espconn_regist_reconcb(&server_conn, tcpserver_reconnect_cb) != 0) {
		ets_uart_printf("Failed to register reconnect callback.\n");
		DEBUG("exit server_init_tcp");
		return -1;
	}

	if (espconn_regist_disconcb(&server_conn, tcpserver_disconnect_cb) != 0) {
		ets_uart_printf("Failed to register disconnect callback.\n");
		DEBUG("exit server_init_tcp");
		return -1;
	}

	server_conn.link_cnt = 0;
	server_conn.reverse = NULL;
	
	espconn_disconnect(&server_conn);

	if ((rc = espconn_accept(&server_conn)) != 0) {
		if (rc == ESPCONN_ISCONN) {
			ets_uart_printf("TCP server already connected.\n");
		} else {
			ets_uart_printf("Failed to accept. %d\n", rc);
			DEBUG("exit server_init_tcp");
			return -1;
		}
	} else {
		/* Set to 0 for unlimited TCP connection time (no timeout) */
		if (espconn_regist_time(&server_conn, 0, 0) != 0) {
			ets_uart_printf("Failed to set timeout interval.\n");
			DEBUG("exit server_init_tcp");
			return -1;
		}
	}

	tcpserver_conn = &server_conn;
	ets_uart_printf("Successfully initialized TCP server.\n\n");
	DEBUG("exit server_init_tcp");
	return 0;
}

static int ICACHE_FLASH_ATTR server_init_udp(uint8 ifnum)
{
	DEBUG("enter server_init_udp");
	static struct ip_info info;
	static struct espconn server_conn;
	static esp_udp server_udp_sta;
	int rc;

	if (!wifi_get_ip_info(ifnum, &info)) {
		ets_uart_printf("Failed to get ip info.\n");
		DEBUG("exit server_init_udp");
		return -1;
	}

	server_udp_sta.local_port = 1025;
	os_memcpy(server_udp_sta.local_ip, &(info.ip.addr), 4);

	server_conn.type = ESPCONN_UDP;
	server_conn.proto.udp = &server_udp_sta;

	server_conn.link_cnt = 0;
	server_conn.reverse = NULL;

	if (espconn_regist_sentcb(&server_conn, udpserver_sent_cb) != 0) {
		ets_uart_printf("Failed to register sent callback.\n");
		DEBUG("exit server_init_udp");
		return -1;
	}

	if (espconn_regist_recvcb(&server_conn, udpserver_recv_cb) != 0) {
		ets_uart_printf("Failed to register recv callback.\n");
		DEBUG("exit server_init_udp");
		return -1;
	}

	if ((rc = espconn_create(&server_conn)) != 0) {
		if (rc == ESPCONN_ISCONN) {
			ets_uart_printf("UDP server already connected.\n");
		} else {
			ets_uart_printf("Failed to create UDP server.\n");
			DEBUG("exit server_init_udp");
			return -1;
		}
	}

	udpserver_conn = &server_conn;
	ets_uart_printf("Successfully initialized UDP server.\n\n");
	DEBUG("exit server_init_udp");
	return 0;
}

int tcpserver_send(struct espconn *conn, void *data, uint16 len, enum Memtype mem)
{
	DEBUG("enter tcpserver_send");
	int rc;
	ets_intr_lock();

	if ((rc = ConnectionTable_sendmsg_push(tcpserver_conn_table, conn, data, len, mem)) != 0) {
		ets_intr_unlock();
		ets_uart_printf("Failed to push data into send queue: %d.\n", rc);
		DEBUG("exit tcpserver_send");
		return -1;
	}

	ets_intr_unlock();
	system_os_post(SERVER_TASK_PRIO, SERVER_SIG_TX, (os_param_t)conn);
	DEBUG("exit tcpserver_send");
	return 0;
}

int ICACHE_FLASH_ATTR sta_server_init()
{
	if (server_init_tcp(STATION_IF) != 0)
		return -1;

	if (server_init_udp(STATION_IF) != 0)
		return -1;

	return 0;
}

int ICACHE_FLASH_ATTR sta_server_close()
{
	if (tcpserver_conn_table == NULL)
		return 0;

	ConnectionTable_destroy(tcpserver_conn_table);
	tcpserver_conn_table = NULL;

	/* Note: Do not call espconn_disconnect. It seems the SDK
		 does not like this, and will fall in an infinite
		 loop and then restart by the watchdog.
		 It may also print an error like "lmac.c 599".
	*/
	tcpserver_conn = NULL;
	udpserver_conn = NULL;
	return 0;
}

int ICACHE_FLASH_ATTR ap_server_init()
{
	if (server_init_tcp(SOFTAP_IF) != 0)
		return -1;

	return 0;
}

int ICACHE_FLASH_ATTR ap_server_close()
{
	if (tcpserver_conn_table == NULL)
		return 0;

	ConnectionTable_destroy(tcpserver_conn_table);
	tcpserver_conn_table = NULL;

	/* Note: Do not call espconn_disconnect. It seems the SDK
		 does not like this, and will fall in an infinite
		 loop and then restart by the watchdog.
		 It may also print an error like "lmac.c 599".
	*/
	tcpserver_conn = NULL;
	return 0;
}
