#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "user_interface.h"

#include "helper.h"
#include "sta_server.h"

extern bool HAS_BEEN_CONNECTED_AS_STATION;
extern bool HAS_RECEIVED_CONNECT_INSTRUCTION;

int ICACHE_FLASH_ATTR start_station(const char *ssid, const char *password)
{
	struct station_config config;

	if (!wifi_set_opmode(STATION_MODE)) {
		ets_uart_printf("Failed to set as station mode.\n");
		return -1;
	}

	os_memset(&config.ssid, 0, 32);
	os_memset(&config.password, 0, 64);

	os_memcpy(&config.ssid, ssid, strlen(ssid));
	os_memcpy(&config.password, password, strlen(password));
	config.bssid_set = 0;
	os_memset(config.bssid, 0, sizeof config.bssid);

	if (!wifi_station_set_config(&config)) {
		ets_uart_printf("Failed to configure station.\n");
		return -1;
	}

	if (!wifi_station_connect()) {
		ets_uart_printf("Failed to connect to router.\n");
		return -1;
	}

	return 0;
}

int ICACHE_FLASH_ATTR start_access_point(const char *ssid, const char *password, uint8 channel)
{
	struct softap_config config;
	ets_uart_printf("Starting access point\n");

	/* Set access point mode */
	if (!wifi_set_opmode(SOFTAP_MODE)) {
		ets_uart_printf("Failed to set as access point mode.\n");
		return -1;
	}

	/* Set ap settings */
	os_memcpy(&config.ssid, ssid, 32);
	os_memcpy(&config.password, password, 64);
	config.ssid_len = strlen(ssid);
	config.channel = channel;
	config.authmode = 0;
	config.ssid_hidden = 0;
	config.max_connection = 10;
	config.beacon_interval = 100;

	if (!wifi_softap_set_config(&config)) {
		ets_uart_printf("Failed to configure access point.\n");
		return -1;
	}

	return 0;
}

/* Set this as wifi handler between system_restart() and resetting the flags
   to avoid getting wifi disconnected events before the flags have been reset
   and get into a restart loop. */
static void ICACHE_FLASH_ATTR empty_wifi_handler(System_Event_t *event)
{
	ets_uart_printf("Empty wifi event handler: event = %d\n", event->event);
}

void ICACHE_FLASH_ATTR sta_wifi_handler(System_Event_t *event)
{
	struct ip_info info;
	
	switch (event->event) {
		case EVENT_STAMODE_CONNECTED:
			ets_uart_printf("Connected to Access Point!\n");
			ets_uart_printf("SSID: %s\n", event->event_info.connected.ssid);
			ets_uart_printf("BSSID: %s\n", str_bssid(event->event_info.connected.bssid));
			ets_uart_printf("Channel: %d\n", event->event_info.connected.channel);
			ets_uart_printf("\n");
			break;

		case EVENT_STAMODE_DISCONNECTED:
			ets_uart_printf("Disconnected from Access Point!\n");
			ets_uart_printf("SSID: %s\n", event->event_info.disconnected.ssid);
			ets_uart_printf("BSSID: %s\n", str_bssid(event->event_info.disconnected.bssid));
			ets_uart_printf("Reason: %d\n", event->event_info.disconnected.reason);
			ets_uart_printf("\n");

			//Retart to discovery mode i.e. AP only
			//Either lost connection or failed to connect to users specified network
			/* Ok so this is tricky... There are two cases where we want to restart.
			   
			1. We were connected as a station to a router and had a valid IP.
				
				In this case we want to restart because the router might be down, and
				we need to try auto-reconnect to it or else timeout and start in access point mode.
				The relevant flag that gets set in this case is HAS_BEEN_CONNECTED_AS_STATION.

			2. We were in access point mode, and have received an instruction to connect to a router,
			   but the connection was unsuccessful, either due to an nonexistent network config,
			   or due to a valid network being temporarily unavailable.
				
				If the config was invalid (bad SSID or password) the device will try to auto-connect
				again on restart and fail. This will cause multiple EVENT_STAMODE_DISCONNECTED events
				to be sent even after the first restart, which we want to ignore after the first restart,
				otherwise we fall into a restart loop. That is, we don't want to restart if the disconnect
				event was due to a connection attempt from the previous system restart, only if it was due
				to a disconnect from the *current* session.

				If the config was valid, but still got disconnected, it could be a temporary router failure.
				When we restart, auto-connect will try to reconnect, and if the router is working again,
				it will succeed next time. If the router is still down, we go back to access point mode after
				the timeout.

				The relevant flag that gets set in this case is HAS_RECEIVED_CONNECT_INSTRUCTION.

			On restart, both flags get set to false again. But we need to disable the current wifi event handler
			to make sure we don't get DISCONNECTED events just after restart, before the flags have been reset.
			We register an empty wifi handler callback just before restarting to avoid this.
			*/
				
			if (HAS_BEEN_CONNECTED_AS_STATION || HAS_RECEIVED_CONNECT_INSTRUCTION) {
				//Disconnected from network - convert to AP for reconfiguration
				if (!wifi_station_disconnect())
					 ets_uart_printf("Failed to disconnect.\n");
	
				wifi_set_event_handler_cb(empty_wifi_handler);
				system_restart();
			}

			break;

		case EVENT_STAMODE_GOT_IP:
			ets_uart_printf("Got IP!\n");
			print_ip_info((struct ip_info *)&(event->event_info.got_ip));
			ets_uart_printf("\n");	
			HAS_BEEN_CONNECTED_AS_STATION = true;
			sta_server_init_tcp();
			sta_server_init_udp();
			break;

		case EVENT_STAMODE_AUTHMODE_CHANGE:
			ets_uart_printf("EVENT_STAMODE_AUTHMODE_CHANGE\n");
			break;

		case EVENT_SOFTAPMODE_STACONNECTED:
			ets_uart_printf("EVENT_SOFTAPMODE_STACONNECTED\n");
			break;

		case EVENT_SOFTAPMODE_STADISCONNECTED:
			ets_uart_printf("EVENT_SOFTAPMODE_STADISCONNECTED\n");			
			break;

		default:
			ets_uart_printf("Got event: %d\n", event->event);
			break;
	}
}
