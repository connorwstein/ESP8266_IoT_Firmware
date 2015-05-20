#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "user_config.h"

void ICACHE_FLASH_ATTR sta_wifi_handler(System_Event_t *event)
{
	struct ip_info info;

	ets_uart_printf("DEBUG: sta_wifi_handler\n");

	switch (event->event) {
		case EVENT_STAMODE_CONNECTED:
			ets_uart_printf("Connected to Access Point!\n");
			ets_uart_printf("SSID: %s\n", event->event_info.connected.ssid);
			ets_uart_printf("BSSID: %02x:%02x:%02x:%02x:%02x:%02x\n",
					event->event_info.connected.bssid[0],
					event->event_info.connected.bssid[1],
					event->event_info.connected.bssid[2],
					event->event_info.connected.bssid[3],
					event->event_info.connected.bssid[4],
					event->event_info.connected.bssid[5]);
			ets_uart_printf("Channel: %d\n", event->event_info.connected.channel);
			ets_uart_printf("\n");
			break;

		case EVENT_STAMODE_GOT_IP:
			ets_uart_printf("Got IP!\n");
			print_ip_info((struct ip_info *)&(event->event_info.got_ip));
			ets_uart_printf("\n");
			break;
		default:
			break;
	}

}
void ICACHE_FLASH_ATTR sta_server_init()
{
	return;
}
