#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"

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
