#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "user_config.h"

#include "ap_server.h"
#include "helper.h"

struct espconn server_conn;
esp_tcp server_tcp;

void ICACHE_FLASH_ATTR init_done()
{
	const char *opmodes[] = {"NONE", "STATION_MODE", "SOFTAP_MODE", "STATIONAP_MODE"};
	struct softap_config config;
	struct ip_info info;
	uint8 opmode;
	enum dhcp_status status;

	ets_uart_printf("System init done.\n");

	opmode = wifi_get_opmode();
	ets_uart_printf("Opmode: 0x%02x (%s)\n", opmode, (opmode < 4 ? opmodes[opmode] : "???"));

	ets_uart_printf("\n");

	if (!wifi_softap_get_config(&config))
		ets_uart_printf("Failed to get wifi softap config.\n");
	else
		print_softap_config(&config);

	ets_uart_printf("\n");

	if (!wifi_get_ip_info(SOFTAP_IF, &info))
		ets_uart_printf("Failed to get ip info.\n");
	else
		print_ip_info(&info);

	ets_uart_printf("\n");

	status = wifi_softap_dhcps_status();

	ets_uart_printf("DHCP server status:\t%d\n", status);
	ets_uart_printf("\n");

	if (ap_server_init() != 0)
		ets_uart_printf("Failed to initialize ap server.\n");
}

void ICACHE_FLASH_ATTR print_greeting()
{
	ets_uart_printf("\n------------------------------\n");
	ets_uart_printf("          GREETINGS!          ");
	ets_uart_printf("\n------------------------------\n");
	ets_uart_printf("\n");
}

void ICACHE_FLASH_ATTR user_init()
{
	char ssid[32] = DEFAULT_AP_SSID;
	char password[64] = DEFAULT_AP_PASSWORD;
	uint8 channel = DEFAULT_AP_CHANNEL;

	system_restore();
	system_set_os_print(0);
	uart_div_modify(0, UART_CLK_FREQ / 115200);

	print_greeting();

	if (start_access_point(ssid, password, channel) != 0)
		ets_uart_printf("Failed to start access point.\n");

	system_init_done_cb(&init_done);
}
