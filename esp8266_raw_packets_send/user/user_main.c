#include "netif.h"
#include "pbuf.h"
#include "osapi.h"
#include "mem.h"

#include "wifi_raw.h"

#define STATION_IF	0x00
#define STATION_MODE	0x01

#define SOFTAP_IF	0x01
#define SOFTAP_MODE	0x02

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

void ICACHE_FLASH_ATTR init_done()
{
	int i;

	os_timer_disarm(&timer);
	os_timer_setfn(&timer, send_paket, NULL);
	os_timer_arm(&timer, 500, 1);
}

void ICACHE_FLASH_ATTR user_init()
{
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	wifi_set_opmode(STATION_MODE);
	system_init_done_cb(init_done);
}
