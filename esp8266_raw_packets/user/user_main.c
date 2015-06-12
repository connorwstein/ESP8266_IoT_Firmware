#include "netif.h"
#include "pbuf.h"
#include "osapi.h"

#define STATION_IF	0x00
#define STATION_MODE	0x01

#define SOFTAP_IF	0x01
#define SOFTAP_MODE	0x02

ETSTimer timer;

void ICACHE_FLASH_ATTR send_paket(void *arg)
{
	struct netif *ifp;
	struct pbuf *pb;
	err_t rc;
	char header[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00};
	char payload[] = {'W', 'A', 'S', 'S', 'U', 'P', ' ', 'P', 'E', 'O', 'P', 'L', 'E', '?'};
	char packet[28];

	os_memcpy(packet, header, 14);
	os_memcpy(packet + 14, payload, 14);

	ifp = (struct netif *)eagle_lwip_getif(SOFTAP_IF);
	((uint32 *)(ifp->state))[11] = 5;

	pb = pbuf_alloc(PBUF_LINK, 100, PBUF_RAM);

	ets_uart_printf("pbuf_alloc\n");

	if (pb == NULL)
		ets_uart_printf("pbuf_alloc failed.\n");

	ets_uart_printf("pbuf_take\n");
	if (pbuf_take(pb, packet, 28) != ERR_OK)
		ets_uart_printf("pbuf_take failed.\n");

	if (rc = ieee80211_output_pbuf(ifp, pb))
		ets_uart_printf("ieee80211_output_pbuf failed: rc = %d\n", rc);
	else
		ets_uart_printf("greeeaaat sucess!\n");

	pbuf_free(pb);
}

void ICACHE_FLASH_ATTR init_done()
{
	int i;

	os_timer_disarm(&timer);
	os_timer_setfn(&timer, send_paket, NULL);
	os_timer_arm(&timer, 1000, 1);
}

void ICACHE_FLASH_ATTR user_init()
{
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	wifi_set_opmode(SOFTAP_MODE);
	wifi_station_set_auto_connect(0);
	system_init_done_cb(init_done);
}
