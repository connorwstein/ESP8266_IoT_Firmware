#include "netif.h"
#include "pbuf.h"
#include "osapi.h"
#include "mem.h"

#define STATION_IF	0x00
#define STATION_MODE	0x01

#define SOFTAP_IF	0x01
#define SOFTAP_MODE	0x02

#define max(x,y) ((x) > (y) ? (x) : (y))

extern void ICACHE_FLASH_ATTR ppTxPkt(void *);

static int called = 0;

/* Warning: this is an experiment, and relies
   on undocumented library calls. Might not work as expected,
   and not guaranteed to work in any other sdk version... */
void ICACHE_FLASH_ATTR aaTxPkt(void *buf, uint16 len)
{
	static int level = 0;
	static void *upper_buf;
	static uint16 upper_len;

	/* If this was called not by us, but by other libraries,
	    just behave exactly like ppTxPkt. */
	if (!called) {
		ppTxPkt(buf);
		return;
	}

	/* level is used to differentiate the two cases where this function is called.
	   This is slightly recursive: aaTxPkt calls ieee80211_output_pbuf which then
	   call aaTxPkt again, this time with different parameters.
	   In the second call to aaTxPkt (from ieee80211_output_pbuf), it calls ppTxPkt. */
	if (level == 0) {
		/* Level 0: we got called by the user. Set up a pbuf struct
			    and call ieee80211_output_pbuf to prepare link layer packet. */
		struct netif *ifp;
		struct pbuf *pb;
		uint8 header[] = {'\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'};
		uint16 alloc_len;

		if (len > 18)
			alloc_len = len - 18;
		else
			alloc_len = 1;

		/* Need to be in SOFTAP_MODE wifi opmode, otherwise this fails! */
		ifp = (struct netif *)eagle_lwip_getif(SOFTAP_IF);
//		ifp = (struct netif *)eagle_lwip_if_alloc(
		pb = pbuf_alloc(PBUF_LINK, alloc_len, PBUF_RAM);

		pbuf_take(pb, header, alloc_len);
		upper_buf = buf;
		upper_len = len;

		level = 1;

		/* Go down one level into ieee80211_output_pbuf. */
		if (ieee80211_output_pbuf(ifp, pb))
			ets_uart_printf("Failed.\n");
		else
			ets_uart_printf("Success!\n");

		level = 0;

		pbuf_free(pb);
	} else {
		/* Got to level 1. This gets called by ieee80211_output_pbuf,
		                   and now the parameters are prepared in the
				   format expected by ppTxPkt. Just modify
				   the packet data in the appropriate memory addresses. */
		int i;

		/* Some debugging stuff... */

		for (i = 0; i < 10; i++)
			ets_uart_printf("%p ", ((uint8 **)buf)[i]);

		ets_uart_printf("\n\n");

		for (i = 0; i < 100; i++)
			ets_uart_printf("%02x ", ((uint8 **)buf)[0][i]);

		ets_uart_printf("\n\n");

		for (i = 0; i < 100; i++)
			ets_uart_printf("%02x ", ((uint8 **)buf)[1][i]);

		ets_uart_printf("\n\n");

		for (i = 0; i < 100; i++)
			ets_uart_printf("%02x ", ((uint8 **)buf)[4][i]);

		ets_uart_printf("\n\n");

		for (i = 0; i < 100; i++)
			ets_uart_printf("%02x ", ((uint8 **)buf)[9][i]);

		ets_uart_printf("\n\n");

//		((uint16 **)buf)[0][4] = upper_len;	/* this doesn't really do anything, though... */
//		((uint16 **)buf)[0][5] = upper_len;	/* this doesn't really do anything, though... */
		memcpy(((uint8 **)buf)[4], upper_buf, upper_len);
		ppTxPkt(buf);
	}
}

/* wrapper around aaTxPkt */
void ICACHE_FLASH_ATTR wifi_send_raw_packet(void *data, uint16 len)
{
	uint8 mode;

	/* Save current opmode and switch to SOFTAP_MODE */
	mode = wifi_get_opmode();
	wifi_set_opmode(SOFTAP_MODE);

	called = 1;
	aaTxPkt(data, len);
	called = 0;

	wifi_set_opmode(mode);
}
