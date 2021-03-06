#include "c_types.h"
#include "osapi.h"
#include "ets_sys.h"
#include "user_interface.h"
#include "user_config.h"

#include "device_config.h"
#include "network_cmds.h"
#include "helper.h"
#include "mqtt.h"
#include "wifi.h"
#include "debug.h"

void ICACHE_FLASH_ATTR init_done()
{
	DEBUG("enter init_done");
	const char *opmodes[] = {"NONE", "STATION_MODE", "SOFTAP_MODE", "STATIONAP_MODE"};
	struct softap_config config;
	struct ip_info info;
	uint8 opmode;
	enum dhcp_status status;

	if (!DeviceInit())
		ets_uart_printf("Failed to init device.\n");

	opmode = wifi_get_opmode();

	ets_uart_printf("Current Opmode: 0x%02x (%s)\n", opmode, (opmode < 4 ? opmodes[opmode] : "???"));
	ets_uart_printf("\n");

#ifdef USE_AS_LOCATOR
	if (opmode == STATIONAP_MODE) {
		if (!wifi_softap_get_config(&config))
			ets_uart_printf("Failed to get wifi locator softap config.\n");
		else
			ets_uart_printf("Locator Access Point SSID: %s\n\n", config.ssid);
	}
#endif
	if (opmode != SOFTAP_MODE) {
		DEBUG("exit init_done");
		return;
	}

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

	DEBUG("exit init_done");
}

void ICACHE_FLASH_ATTR user_init()
{
	DEBUG("enter user_init");
	struct DeviceConfig conf;
	
//	system_restore();
//	system_set_os_print(0);
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	ets_uart_printf("\n\n\n");
	ets_uart_printf("Free heap: %u\n", system_get_free_heap_size());

	if (DeviceConfig_already_exists()) {
		if (DeviceConfig_read_config(&conf) != 0) {
			ets_uart_printf("Failed to read device config.\n");
		} else {
			ets_uart_printf("Current device config: device_name = %s, device_room = %s, device_type = %s\n",
					conf.name, conf.room, DeviceConfig_strtype(conf.type));
		}

		DeviceConfig_delete(&conf);
	} else {
		ets_uart_printf("This device has not yet been configured.\n");
	}

	wifi_init();
	mqtt_init();

	system_init_done_cb(&init_done);
	DEBUG("exit user_init");
}
