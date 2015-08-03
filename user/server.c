#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "mem.h"

#include "device_config.h"
#include "helper.h"
#include "message_queue.h"
#include "parser.h"

#include "debug.h"

#define TCP_MAX_PACKET_SIZE		1460

#define SERVER_TASK_PRIO		1
#define SERVER_TASK_QUEUE_LEN		20

#define SERVER_SIG_RX			1
#define SERVER_SIG_TX			2
#define SERVER_SIG_TX_DONE		3

static struct MessageQueue sendq = MESSAGE_QUEUE_INITIALIZER(sendq);
static struct MessageQueue recvq = MESSAGE_QUEUE_INITIALIZER(recvq);

static os_event_t server_task_queue[SERVER_TASK_QUEUE_LEN];

static void ICACHE_FLASH_ATTR server_task(os_event_t *e)
{
	DEBUG("enter server_task");
	static bool sending = false;
	static bool more_fragments = false;
	static struct espconn *sending_conn;
	static uint8 *first_sending_chunk;
	static uint8 *current_sending_chunk;
	static uint16 sending_len;
	static enum Memtype sending_mem;

	struct espconn *conn;
	void *data;
	uint16 len;
	enum Memtype mem;

	switch (e->sig) {
		case SERVER_SIG_RX:
			ets_intr_lock();

			if (MessageQueue_unshift(&recvq, &conn, &data, &len, &mem) == 0) {
				ets_intr_unlock();
				tcpparser_process_data(data, len, conn);

				if (mem == HEAP_MEM)
					os_free(data);
			} else {
				ets_intr_unlock();
			}

			break;

		case SERVER_SIG_TX:
			ets_intr_lock();

			if (!sending) {
				if (MessageQueue_unshift(&sendq, &conn, &data, &len, &mem) == 0) {
					sending = true;
					ets_intr_unlock();

					if (len <= TCP_MAX_PACKET_SIZE) {
						more_fragments = false;

						if (espconn_sent(conn, data, len) != ESPCONN_OK)
							ets_uart_printf("Failed to send data.\n\n");

						if (mem == HEAP_MEM)
							os_free(data);
					} else {
						more_fragments = true;
						sending_conn = conn;
						first_sending_chunk = (uint8 *)data;
						current_sending_chunk = (uint8 *)data;
						sending_len = len;
						sending_mem = mem;

						if (espconn_sent(conn, data, TCP_MAX_PACKET_SIZE) != ESPCONN_OK)
							ets_uart_printf("Failed to send data.\n\n");

						current_sending_chunk += TCP_MAX_PACKET_SIZE;
						sending_len -= TCP_MAX_PACKET_SIZE;
						ets_uart_printf("More data to send.\n");
					}
				} else {
					ets_intr_unlock();
				}
			} else {
				ets_intr_unlock();
			}

			break;

		case SERVER_SIG_TX_DONE:
			ets_intr_lock();

			if (sending && more_fragments) {
				ets_intr_unlock();

				if (sending_len <= TCP_MAX_PACKET_SIZE) {
					more_fragments = false;

					if (espconn_sent(sending_conn, current_sending_chunk, sending_len) != ESPCONN_OK)
						ets_uart_printf("Failed to send data.\n\n");

					if (sending_mem == HEAP_MEM)
						os_free(first_sending_chunk);

					ets_intr_lock();
					sending = false;
					ets_intr_unlock();
				} else {
					more_fragments = true;

					if (espconn_sent(sending_conn, current_sending_chunk, TCP_MAX_PACKET_SIZE) != ESPCONN_OK)
						ets_uart_printf("Failed to send data.\n\n");

					current_sending_chunk += TCP_MAX_PACKET_SIZE;
					sending_len -= TCP_MAX_PACKET_SIZE;
					ets_uart_printf("More data to send.\n");
				}
			} else if (MessageQueue_empty(&sendq)) {
				sending = false;
				ets_intr_unlock();
			} else {
				sending = false;
				ets_intr_unlock();
				system_os_post(SERVER_TASK_PRIO, SERVER_SIG_TX, 0);
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

	data = (char *)os_zalloc(len + 1);	/* Leave 1 byte for NULL char */

	if (data == NULL) {
		ets_uart_printf("Failed to alloc memory for recieved data.\n\n");
		DEBUG("exit tcpserver_recv_cb");
		return;
	}

	/* Need to copy it because the pdata might get freed after exiting the callback. */
	os_memcpy(data, pdata, len);

	ets_intr_lock();

	if (MessageQueue_push(&recvq, (struct espconn *)arg, data, len, HEAP_MEM) != 0) {
		ets_intr_unlock();
		ets_uart_printf("Failed to push recieved message in recv message queue.\n\n");
		return;
	}

	ets_intr_unlock();

	/* Post TCP_SIG_RX to server task. */
	system_os_post(SERVER_TASK_PRIO, SERVER_SIG_RX, 0);

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

	udpparser_process_data(pdata, len, arg);
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

	/* Post TCP_SIG_TX_DONE to server task. */
	system_os_post(SERVER_TASK_PRIO, SERVER_SIG_TX_DONE, 0);

	ets_uart_printf("\n");
	DEBUG("exit tcpserver_sent_cb");
}

static void ICACHE_FLASH_ATTR udpserver_sent_cb(void *arg)
{
	DEBUG("enter udpserver_sent_cb");
	uint32 remote_ip;
	int remote_port;

	ets_uart_printf("udpserver_sent_cb\n");
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

	ets_uart_printf("\n");
	DEBUG("exit tcpserver_connect_cb");
}

static void ICACHE_FLASH_ATTR tcpserver_reconnect_cb(void *arg, sint8 err)
{
	DEBUG("enter tcpserver_reconnect_cb");
	ets_uart_printf("Reconnect: err = %d\n", err);
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

	MessageQueue_clear(&sendq);
	MessageQueue_clear(&recvq);
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

//	tcpserver_conn = &server_conn;
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

//	udpserver_conn = &server_conn;
	ets_uart_printf("Successfully initialized UDP server.\n\n");
	DEBUG("exit server_init_udp");
	return 0;
}

int tcpserver_send(struct espconn *conn, void *data, uint16 len, enum Memtype mem)
{
	DEBUG("enter tcpserver_send");
	ets_intr_lock();

	if (MessageQueue_push(&sendq, conn, data, len, mem) != 0) {
		ets_intr_unlock();
		ets_uart_printf("Failed to push data into send queue.\n");
		DEBUG("exit tcpserver_send");
		return -1;
	}

	ets_intr_unlock();
	system_os_post(SERVER_TASK_PRIO, SERVER_SIG_TX, 0);
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

void ICACHE_FLASH_ATTR sta_server_close()
{
	MessageQueue_clear(&sendq);
	MessageQueue_clear(&recvq);

	/* Note: Do not call espconn_disconnect. It seems the SDK
		 does not like this, and will fall in an infinite
		 loop and then restart by the watchdog.
		 It may also print an error like "lmac.c 599".
	*/
//	tcpserver_conn = NULL;
//	udpserver_conn = NULL;
}

int ICACHE_FLASH_ATTR ap_server_init()
{
	if (server_init_tcp(SOFTAP_IF) != 0)
		return -1;

	return 0;
}

void ICACHE_FLASH_ATTR ap_server_close()
{
	MessageQueue_clear(&sendq);
	MessageQueue_clear(&recvq);

	/* Note: Do not call espconn_disconnect. It seems the SDK
		 does not like this, and will fall in an infinite
		 loop and then restart by the watchdog.
		 It may also print an error like "lmac.c 599".
	*/
//	tcpserver_conn = NULL;
}
