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

struct beacon_frame {
	uint16 fc;
	uint16 duration_id;
	uint8 addr1[6];
	uint8 addr2[6];
	uint8 addr3[6];
	uint16 sequence_control;
	uint8 timestamp[8];
	uint16 beacon_int;
	uint16 cap_info;
	uint8 element_id;
	uint8 length;
	uint8 ssid[32];
};

ETSTimer timer;

/* function that sends the raw packet.
   Note: the actual packet sent over the air may be longer because
   the driver functions seem to allocate memory for the whole IEEE-802.11
   header even if the packet is shorter...

   Currently, packets of length less than or equal to 18 bytes have
   additional bytes over the air until it reaches 19 bytes. For larger packets,
   the length should be correct. */
void ICACHE_FLASH_ATTR send_packet(void *arg)
{
	char packet[64];

	packet[0] = '\xde';
	packet[1] = '\xad';
	packet[2] = '\xbe';	/* This will become \x00 */
	packet[3] = '\xef';	/* This too. */
	os_sprintf(packet + 4, "%s%s%s%s%s", "HELLO WORLD!", "HELLO WORLD!", "HELLO WORLD!", "HELLO WORLD!", "HaLLO w0RLD!");

	wifi_send_raw_packet(packet, sizeof packet);
}

void ICACHE_FLASH_ATTR send_beacon(void *arg)
{
	struct beacon_frame frame;
	//uint8 packet[42];

	frame.fc = 0x0080;
	frame.duration_id = 0x0000;
	os_memcpy(frame.addr1, "\xff\xff\xff\xff\xff\xff", 6);
	wifi_get_macaddr(SOFTAP_IF, frame.addr2);
	wifi_get_macaddr(SOFTAP_IF, frame.addr3);
	frame.sequence_control = 0x0000;
	os_memset(frame.timestamp, 0, 8);
	*((uint32 *)(frame.timestamp)) = (NOW() << 4) / 5;
	frame.beacon_int = 976;
	frame.cap_info = 0x0001;
	frame.element_id = 0;
	frame.length = 4;
	os_memset(frame.ssid, 0, 32);
	os_memcpy(frame.ssid, "ABCD", 4);

	wifi_send_raw_packet(&frame, 42);
}

void ICACHE_FLASH_ATTR my_recv_cb(struct RxPacket *pkt)
{
	static int counter = 0;
	uint16 len;
	uint16 i, j;

	len = pkt->rx_ctl.legacy_length;
	ets_uart_printf("Recv callback #%d: %d bytes\n", counter++, len);
	ets_uart_printf("Channel: %d PHY: %d\n", pkt->rx_ctl.channel, wifi_get_phy_mode());

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

	if (len % 16 != 0) {
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

		ets_uart_printf("\n");
	}

	ets_uart_printf("\n");
}

void ICACHE_FLASH_ATTR init_done()
{
	// Note that it is impossible to see all packets
	// on all channels and physical modes
	// Select a phy 802.11 b/g/n etc. and a channel 
	// in order to receive packets.
	// Note: ESP8266 does not appear to support 5GHz.
	wifi_set_channel(6);
	wifi_set_phy_mode(2);
	/* Note: it appears the channel might get reset to default (6)
		 after a wifi_set_opmode call (maybe, we aren't sure
		 if that's the case). Also, we got some watchdog resets once. */
	os_timer_disarm(&timer);
	os_timer_setfn(&timer, send_beacon, NULL);
	os_timer_arm(&timer, 1000, 1);
//	wifi_raw_set_recv_cb(my_recv_cb);
}

void ICACHE_FLASH_ATTR user_init()
{
	system_set_os_print(0);
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	wifi_set_opmode(STATION_MODE);
	system_init_done_cb(init_done);
}
