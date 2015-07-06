#include "c_types.h"
#include "eagle_soc.h"
#include "ets_sys.h"
#include "gpio.h"
#include "osapi.h"
#include "user_interface.h"

#define TOTICKS(x) ((x)*5)>>4

uint32 baud = 38400;
uint32 delay = 8;


void ICACHE_FLASH_ATTR bit_bang_send(const char *data, uint16 len)
{
	char byte;
	uint16 i, j;

	for (i = 0; i < len; i++) {
		byte = data[i];
		j = 0;

		/* Start bit */
		GPIO_OUTPUT_SET(4, 0);
		os_delay_us(delay);

		/* Data bits */
		while (j < 8) {
			/* ADD parentheses here since macro doesn't.
				Precedence is fucked up. */
			GPIO_OUTPUT_SET(4, (byte & 0x1));
			os_delay_us(delay);
			byte >>= 1;
			j++;
		}

		/* Stop bit */
		GPIO_OUTPUT_SET(4, 1);
		os_delay_us(delay);
	}

	GPIO_DIS_OUTPUT(4);
}

#if 1
void bit_bang_read_byte(uint32 intr_mask, void *arg)
{
	uint32 clock;
	uint8 byte = -1;
	uint8 bit, bit1;
	int i;
	
	
	/* Check if this is a start bit */
	/* Record current clock time */
	clock = NOW();

	/* Check that the signal is stable by waiting for half the bit time. */
	while (((bit = GPIO_INPUT_GET(5)) == 0) && (NOW() - clock < TOTICKS(delay / 2)));

	/* If signal went back to 1, assume it's noise and return. */
	if (bit == 1)
		// ets_uart_printf("Noise on start bit\n");
		return;

	ets_uart_printf("Read callback\n");

	/* Wait for the rest of the bit time */
	while (NOW() - clock < TOTICKS(delay));

	clock = NOW();

	/* Read data bits */
	for (i = 0; i < 8; i++) {
		/* Read current data bit value */
		bit = GPIO_INPUT_GET(5);

		/* Check that the data bit value remains stable for half the bit time */
		while (((bit1 = GPIO_INPUT_GET(5)) == bit1) && (NOW() - clock < TOTICKS(delay / 2)));

		/* If signal changed. */
		if (bit1 != bit)
			ets_uart_printf("Noisey bit\n");
			return;

		/* Wait for the rest of the bit time */
		while (NOW() - clock < TOTICKS(delay / 2));

		clock = NOW();

		/* Shift in the data bit */
		byte <<= 1;
		byte |= bit;
	}

	/* Wait for stop bit */
	/* Wait until we get a HIGH signal */
	while ((bit = GPIO_INPUT_GET(5)) == 0 && (NOW() - clock < TOTICKS(delay / 2)));

	/* If we haven't received the stop bit for half a bit time, return. */
	if (NOW() - clock >= TOTICKS(delay / 2))
		ets_uart_printf("No stop bits\n");
		return;

	/* Record current clock time */
	clock = NOW();

	/* Check that the signal is stable by waiting for half the bit time. */
	while (((bit = GPIO_INPUT_GET(5)) == 1) && (NOW() - clock < TOTICKS(delay / 2)));

	/* If signal went back to 0, assume it's noise and return. */
	if (bit == 0)
		ets_uart_printf("Stop bit noise\n");
		return;

	if (arg == NULL)
		ets_uart_printf("Null arg\n");
		return;

	ets_uart_printf("Successfully received byte: %c\n",byte);
	// if (((struct rx_buffer *)arg)->len == ((struct rx_buffer *)arg)->max)
	// 	return;

	// ((struct rx_buffer *)arg)->data[((struct rx_buffer *)arg)->len++] = byte;
	// ((struct rx_buffer *)arg)->read_done_cb();
}

#endif
void ICACHE_FLASH_ATTR init_done()
{
	char *string = "HELLO WORLD!\n";
	char data[10];
	int i;

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO4_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);
	GPIO_DIS_OUTPUT(5);

	ETS_GPIO_INTR_ATTACH(bit_bang_read_byte, NULL);
	gpio_pin_intr_state_set(GPIO_ID_PIN(5), GPIO_PIN_INTR_NEGEDGE);
	ETS_GPIO_INTR_ENABLE();

	for (i = 0; i < 10; i++) {
		bit_bang_send(string, strlen(string));
		os_delay_us(1000000);
	}
}

void ICACHE_FLASH_ATTR user_init()
{
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	system_set_os_print(0);
	ets_wdt_disable();

	gpio_init();
	system_init_done_cb(init_done);
}
