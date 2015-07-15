#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "mem.h"

#include "device_config.h"
#include "lighting.h"
#include "debug.h"

static enum LightState state = LIGHT_OFF;

void ICACHE_FLASH_ATTR Lighting_toggle_light()
{
	DEBUG("enter Lighting_toggle_light");
	if (state == LIGHT_OFF)
		state = LIGHT_ON;
	else
		state = LIGHT_OFF;

	ets_uart_printf("Toggled light to %s!\n", state == LIGHT_OFF ? "OFF" : "ON");

	if (DeviceConfig_set_data(&state, sizeof (enum LightState)) != 0)
		ets_uart_printf("Failed to set lighting data in device config.\n");

	DEBUG("exit Lighting_toggle_light");
}

void ICACHE_FLASH_ATTR Lighting_get_light(struct espconn *conn)
{
	DEBUG("enter Lighting_get_light");
	char buff[5];

	memset(buff, 0, sizeof buff);
	os_sprintf(buff, "%s", state == LIGHT_OFF ? "OFF" : "ON");

	if (espconn_sent(conn, (uint8 *)buff, os_strlen(buff)) != 0)
		ets_uart_printf("espconn_sent failed.\n");

	ets_uart_printf("Replied with: %s\n", buff);
	DEBUG("exit Lighting_get_light");
}

int ICACHE_FLASH_ATTR Lighting_init(struct DeviceConfig *config)
{
	if (config->data_len != sizeof (enum LightState)) {
		ets_uart_printf("Wrong size for lighting data: %d.\n", config->data_len);
		return -1;
	}

	state = *(enum LightState *)(config->data);
	ets_uart_printf("Lighting initialized: state = %d!\n", state);
	return 0;
}

int ICACHE_FLASH_ATTR Lighting_set_default_data(struct DeviceConfig *config)
{
	config->data = (enum LightState *)os_zalloc(sizeof (enum LightState));

	if (config->data == NULL) {
		ets_uart_printf("Failed to allocate memory for lighting data.\n");
		return -1;
	}

	config->data_len = sizeof (enum LightState);
	*(enum LightState *)(config->data) = LIGHT_OFF;
	return 0;
}
