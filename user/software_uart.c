#include "c_types.h"
#include "eagle_soc.h"
#include "ets_sys.h"
#include "gpio.h"
#include "osapi.h"
#include "mem.h"

#include "software_uart.h"

/* Converts between microseconds and timer clock ticks.
   The NOW() macro returns time in terms of timer clock ticks.
   The timer clock has a frequency of TIMER_CLK_FREQ = 312500 Hz
   (see eagle_soc.h or ets_sys.h). */
#define TOTICKS(x) (((x) * 5) >> 4)

#define BITS_IN_BYTE 8

static uint32 baud_rate = 38400;
static uint32 bit_delay = 26;	/* In microseconds */

static uint8 gpio_tx_pin = 4;
static uint8 gpio_rx_pin = 5;

static uint16 byte_count = 0;
static struct rx_buffer *buffer = NULL;

void ICACHE_FLASH_ATTR bit_bang_send(const char *data, uint16 len)
{
	uint32 clock;
	char byte;
	uint16 i, j;
	
	gpio_pin_intr_state_set(gpio_rx_pin, GPIO_PIN_INTR_DISABLE);
//	ets_uart_printf("gpio_tx_pin = %d, gpio_rx_pin = %d, baud_rate = %d, bit_delay = %d\n",
//			 gpio_tx_pin, gpio_rx_pin, baud_rate, bit_delay);

//	ets_uart_printf("Sending %02X %02X %02X %02X\n", data[0], data[1], data[2], data[3]);
//	os_delay_us(1000);

	for (i = 0; i < len; i++) {
		byte = data[i];
		j = 0;

		clock = NOW();

		/* Start bit */
		GPIO_OUTPUT_SET(gpio_tx_pin, 0);
//		os_delay_us(bit_delay);
		while ((NOW() - clock) < TOTICKS(bit_delay));

		/* Data bits */
		while (j < 8) {
			clock = NOW();

			/* ADD parentheses here since macro doesn't.
				Precedence is messed up. */
			GPIO_OUTPUT_SET(gpio_tx_pin, (byte & 0x1));
//			os_delay_us(bit_delay - 1); /* If you use os_delay, make sure this is -1, or else might fail. */
			while ((NOW() - clock) < TOTICKS(bit_delay));

			byte >>= 1;
			j++;
		}

		clock = NOW();

		/* Stop bit */
		GPIO_OUTPUT_SET(gpio_tx_pin, 1);
//		os_delay_us(bit_delay);
		while ((NOW() - clock) < TOTICKS(bit_delay));
	}

	gpio_pin_intr_state_set(gpio_rx_pin, GPIO_PIN_INTR_NEGEDGE); 	
}

void bit_bang_read_byte(uint32 intr_mask, void *arg)
{
	uint32 clock = NOW();
	int i = 0;
	uint8 byte = 0;

	gpio_intr_ack(intr_mask);
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);	

	if (gpio_status == 0)
		return; // Not actual data when gpio_status is 0

	while ((NOW() - clock) < TOTICKS(bit_delay + bit_delay / 2)); //wait until middle of first bit

	clock = NOW();
	
	while (i < BITS_IN_BYTE) {
		//loop through next 8 bits, putting each read into a single bit of "byte"
		byte = (byte >> 1) | (GPIO_INPUT_GET(gpio_rx_pin) << (BITS_IN_BYTE - 1));

		while ((NOW() - clock) < TOTICKS(bit_delay));

		clock = NOW();
		i++;
	}

	buffer->buf[byte_count++] = byte;

	if (byte_count == buffer->size) {
		disable_interrupts();
		ets_uart_printf("Finished reading data\n");

		if (buffer->recv_cb != NULL)
			buffer->recv_cb(buffer);
	}

	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
}

void ICACHE_FLASH_ATTR enable_interrupts()
{
	ets_uart_printf("buffer size in enable interrupts: %d\n", buffer->size);
	//ets_uart_printf("Enabling interrupts \n");
	ETS_GPIO_INTR_ATTACH(bit_bang_read_byte, NULL);
	ETS_GPIO_INTR_DISABLE(); 
	ETS_GPIO_INTR_ENABLE();
	//ets_uart_printf("After enabled interrupts\n");
}

void ICACHE_FLASH_ATTR disable_interrupts()
{
	gpio_pin_intr_state_set(gpio_rx_pin, GPIO_PIN_INTR_DISABLE);
	ETS_GPIO_INTR_DISABLE();
}

bool ICACHE_FLASH_ATTR read_buffer_full()
{
	return byte_count < buffer->size ? false : true;
}

void ICACHE_FLASH_ATTR flush_read_buffer()
{
	byte_count = 0;
	os_memset(buffer->buf, 0, buffer->size);
	buffer = NULL;
}

void ICACHE_FLASH_ATTR set_rx_buffer(struct rx_buffer *buffer_set)
{
	buffer = buffer_set;
	byte_count = 0;
}

void ICACHE_FLASH_ATTR software_uart_config(uint32 baud, uint8 rx_pin, uint8 tx_pin)
{
	if (baud == 0) {
		ets_uart_printf("Invalid baud of 0.\n");
		return;
	}

	baud_rate = baud;
	bit_delay = 1000000 / baud;
	gpio_rx_pin = rx_pin;
	gpio_tx_pin = tx_pin;
}

struct rx_buffer *ICACHE_FLASH_ATTR create_rx_buffer(uint16 response_size, rx_buffer_recv_cb recv_cb)
{
	uint8 *buffer;
	struct rx_buffer *buffer_struct;

	buffer = (uint8 *)os_zalloc(response_size);

	if (buffer == NULL)
		return NULL;

	buffer_struct = (struct rx_buffer *)os_zalloc(sizeof (struct rx_buffer));

	if (buffer_struct == NULL) {
		os_free(buffer);
		return NULL;
	}

	buffer_struct->size = response_size;
	buffer_struct->buf = buffer;
	buffer_struct->recv_cb = recv_cb;
	return buffer_struct;
}

void ICACHE_FLASH_ATTR destroy_rx_buffer(struct rx_buffer *buffer_to_kill)
{
	flush_read_buffer(); 

	if (buffer_to_kill == NULL)
		return;

	os_free(buffer_to_kill->buf);
	os_free(buffer_to_kill);
}
