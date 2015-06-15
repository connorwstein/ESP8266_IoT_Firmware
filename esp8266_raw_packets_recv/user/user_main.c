#include "c_types.h"

#include "wifi_raw.h"



void ICACHE_FLASH_ATTR aaEnqueueRxq(void *a)
{
/*	int i;

	for (i = 0; i < 10; i++)
		ets_uart_printf("%p ", ((void **)a)[i]);

	ets_uart_printf("\n");

	for (i = 0; i < 100; i++)
		ets_uart_printf("%02x ", ((uint8 **)a)[4][i]);

	ets_uart_printf("\n\n");
*/
	ppEnqueueRxq(a);
}

void ICACHE_FLASH_ATTR user_init()
{
	uart_div_modify(0, 80 * 1000000 / 115200);
}
