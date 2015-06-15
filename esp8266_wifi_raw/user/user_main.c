#define LWIP_OPEN_SRC

#include "netif.h"
#include "pbuf.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"

#include "wifi_raw.h"

#define STATION_IF	0x00
#define STATION_MODE	0x01

#define SOFTAP_IF	0x01
#define SOFTAP_MODE	0x02

static int count=0;

ETSTimer timer;

/* function that send the raw packet.
   Note: the actual packet sent over the air may be longer because
   the driver functions seem to allocate memory for the whole IEEE-802.11
   header even if the packet is shorter...

   Currently, packets of length less than or equal to 18 bytes have
   additional bytes over the air until it reaches 19 bytes. For larger packets,
   the length should be correct. */
void ICACHE_FLASH_ATTR send_paket(void *arg)
{
	char paket[64];

	paket[0] = '\xde';
	paket[1] = '\xad';
	paket[2] = '\xbe';	/* This will become \x00 */
	paket[3] = '\xef';	/* This too. */
	os_sprintf(paket + 4, "%s%s%s%s%s", "HELLO WORLD!", "HELLO WORLD!", "HELLO WORLD!", "HELLO WORLD!", "HaLLO w0RLD!");

	wifi_send_raw_packet(paket, sizeof paket);
}

void ICACHE_FLASH_ATTR my_recv_cb(struct RxPacket *pkt)
{
	uint16 len;
	uint16 i, j;

	len = pkt->rx_ctl.legacy_length;
	ets_uart_printf("Recv callback: %d bytes, %d Channel: %d PHY: %d\n", len, count,pkt->rx_ctl.channel, wifi_get_phy_mode());
	count++;
	i = 0;
	while (i < len / 16) {
		ets_uart_printf("0x%04x: ", i);

		for (j = 0; j < 8; j++) {
			ets_uart_printf("%02x", pkt->data[16 * i + 2 * j]);
			ets_uart_printf("%02x ", pkt->data[16 * i + 2 * j + 1]);
		}

		ets_uart_printf("\t");

		for (j = 0; j < 16; j++) {
			if ((pkt->data[16 * i + j] >= ' ') && (pkt->data[16 * i + j] <= '~'))
				ets_uart_printf("%c", pkt->data[16 * i + j]);
			else
				ets_uart_printf(".");
		}

		ets_uart_printf("\n");
		++i;
	}

	ets_uart_printf("0x%04x: ", i);

	for (j = 0; j < len % 16; j++) {
		ets_uart_printf("%02x", pkt->data[16 * i + j]);

		if (j % 2 == 1)
			ets_uart_printf(" ");
	}

	for (; j < 16; j++) {
		ets_uart_printf(" ");
		ets_uart_printf(" ");

		if (j % 2 == 1)
			ets_uart_printf(" ");
	}

	ets_uart_printf("\t");

	for (j = 0; j < len % 16; j++) {
		if ((pkt->data[16 * i + j] >= ' ') && (pkt->data[16 * i + j] <= '~'))
			ets_uart_printf("%c", pkt->data[16 * i + j]);
		else
			ets_uart_printf(".");
	}

	ets_uart_printf("\n\n");
}

void ICACHE_FLASH_ATTR init_done()
{
	//Note that it is impossible to see all packets
	//on all channels and physical modes
	//Select a phy 80211 b/g/n/a etc. and a channel 
	//in order to receive packets
	wifi_set_channel(6);
	wifi_set_phy_mode(2);
	os_timer_disarm(&timer);
	os_timer_setfn(&timer, send_paket, NULL);
	os_timer_arm(&timer, 500, 1);
	wifi_raw_set_recv_cb(my_recv_cb);
}

void ICACHE_FLASH_ATTR user_init()
{
	system_set_os_print(0);
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	wifi_set_opmode(STATION_MODE);
	system_init_done_cb(init_done);
}
