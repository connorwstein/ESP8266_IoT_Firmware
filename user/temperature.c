#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"

#include "debug.h"

static int temperature = 0;

void ICACHE_FLASH_ATTR Temperature_set_temperature(int temp)
{
	DEBUG("enter Temperature_set_temperature");
	temperature = temp;
	ets_uart_printf("Set TEMPERATURE to %d\n", temperature);
	DEBUG("exit Temperature_set_temperature");
}

void ICACHE_FLASH_ATTR Temperature_get_temperature(struct espconn *conn)
{
	DEBUG("enter Temperature_get_temperature");
	char buff[20];

	os_sprintf(buff, "%d", temperature);

	if (espconn_sent(conn, (uint8 *)buff, os_strlen(buff)) != 0)
		ets_uart_printf("espconn_sent failed.\n");

	ets_uart_printf("Replied with: %s\n", buff);
	DEBUG("exit Temperature_get_temperature");
}
