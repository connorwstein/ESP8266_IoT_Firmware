#define LWIP_OPEN_SRC

#include "netif.h"
#include "pbuf.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"

#include "wifi_raw.h"
#include "header.h"

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

	wifi_send_raw_packet(packet, 27);
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

void ICACHE_FLASH_ATTR test_timer(void *arg)
{
	static uint8 *data = "HELLO";
	static struct pbuf pb;
	struct esf_buf *buf;
	int rc;

	os_memset(&pb, 0, sizeof pb);
	pb.len = 5;
	pb.payload = data;

	buf = (struct esf_buf *)esf_buf_alloc(&pb, 1);
	ets_uart_printf("Free heap size: %u\n", system_get_free_heap_size());

	buf->ep->data[4] = 0x00;
	buf->ep->data[6] = 0x20;
//	buf->e_data->i_fc[0] = 0x80;

	ets_uart_printf("buf = %p\n", buf);
	rc = ppTxPkt(buf);
	ets_uart_printf("ppTxPkt: %d\n", rc);

//	esf_buf_recycle(buf, 1);
	ets_uart_printf("exit timer\n");
}

static int he_called = 0;
int aaProcessTxQ(uint8 sig)
{
	ppProcessTxQ(sig);
/*
	int rc;
	he_called = 1;
	ets_uart_printf("In ppProcessTxQ: sig = %d\n", sig);
	ets_uart_printf("return ppProcessTxQ = %d\n\n", rc = ppProcessTxQ(sig));
	he_called = 0;
	return rc;
*/
}

void amacTxFrame(struct esf_buf *arg1, uint8 arg2)
{
//	lmacTxFrame(arg1, arg2);
/*
	if (he_called) {
*/		ets_uart_printf("In lmacTxFrame: arg1 = %p, arg2 = %d\n", arg1, arg2);
		print_esf_buf((void *)arg1);
		ets_uart_printf("return lmacTxFrame = %d\n", lmacTxFrame((void *)arg1, arg2));
/*	} else {
		lmacTxFrame(arg1, arg2);
	}
*/
}

void ICACHE_FLASH_ATTR test_lmac(void *arg)
{
	static struct esf_buf ebuf;
	static struct _ebuf_sub1 ep;
	static uint8 ep_data[24] = {0x82, 0x02, 0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0xad, 0x10, 0x01, 0x00, 0x00, 0x00, 0x90};//{0x86, 0x02, 0x08, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xca, 0x01, 0x00, 0x00, 0x00, 0x00, 0x32, 0x9f, 0x4e, 0x04, 0xd4, 0x81, 0xfe, 0x90};
	static uint8 data[] = "HELLOAAAAAAAAAAAAAAAAAAAAAAA";

	os_memcpy(ep.data, ep_data, sizeof ep.data);
	os_memset(&ebuf, 0, sizeof ebuf);

	ebuf.cnt1 = 1;
	ebuf.e_data = (void *)data;
	ebuf.len1 = 24;
	ebuf.len2 = 8;
	ebuf.ep = &ep;

//	amacTxFrame((uint8 *)&ebuf, 0);
}

void ICACHE_FLASH_ATTR test_pp(void *arg)
{
	struct pbuf *pb;
	struct esf_buf *ebuf;
	uint8 ep_data[24] = {0x82, 0x02, 0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0xad, 0x10, 0x01, 0x00, 0x00, 0x00, 0x90};
	pb = pbuf_alloc(PBUF_LINK, 28, PBUF_RAM);
	pbuf_take(pb, "HELLOWORLD", 28);

	ebuf = (struct esf_buf *)esf_buf_alloc(pb, 1);
	os_memcpy(ebuf->ep->data, ep_data, 16);
	print_esf_buf(ebuf);
	ppTxPkt(ebuf);
}

void ICACHE_FLASH_ATTR task_32(struct ETSEventTag *e)
{
	ets_uart_printf("In task 32: sig = %d\n", e->sig);
}

void ICACHE_FLASH_ATTR init_done()
{
	static struct ETSEventTag queue[34];

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
	os_timer_setfn(&timer, test_pp, NULL);
	os_timer_arm(&timer, 1000, 1);
//	wifi_raw_set_recv_cb(my_recv_cb);
//	ets_task(task_32, 32, queue, 34);
}

void ICACHE_FLASH_ATTR user_init()
{
	system_set_os_print(0);
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	wifi_set_opmode(STATION_MODE);
	system_init_done_cb(init_done);
}
