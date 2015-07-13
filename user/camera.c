#include "c_types.h"
#include "eagle_soc.h"
#include "gpio.h"
#include "ets_sys.h"
#include "mem.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"

#include "software_uart.h"
#include "sta_server.h"

#define RESET_RESPONSE_SIZE		4
#define TAKE_PICTURE_RESPONSE_SIZE	5
#define READ_SIZE_RESPONSE_SIZE		9
#define READ_CONTENT_RESPONSE_SIZE	5

static uint32 baud_rate = 38400;
static uint8 gpio_camera_rx;
static uint8 gpio_camera_tx;

struct espconn *reply_conn = NULL;

static void ICACHE_FLASH_ATTR print_rx_buffer(rx_buffer *buffer)
{
	int j = 0;
	ets_uart_printf("Response: ");

	while (j < buffer->size)
		ets_uart_printf("%02x ", buffer->buf[j++]);

	ets_uart_printf("\n");
}

static void camera_picture_recv_done(rx_buffer *buffer)
{
	uint16 len;
	uint16 offset;
	int rc;

	ets_uart_printf("Done receiving picture contents: %d bytes\n", buffer->size);
	ets_uart_printf("Sending data to phone.\n");

	sta_server_send_large_buffer(reply_conn, buffer->buf, buffer->size);
	os_free(buffer);
}

int ICACHE_FLASH_ATTR camera_reset()
{
	uint8 command[] = {'\x56', '\x00', '\x26', '\x00'};
	uint8 success[] = {'\x76', '\x00', '\x26', '\x00'};
	rx_buffer *reset_buffer;

	ets_uart_printf("Resetting camera...\n");
	reset_buffer = create_rx_buffer(RESET_RESPONSE_SIZE, NULL);

	if (reset_buffer == NULL)
		return -1;

	enable_interrupts(gpio_camera_tx, reset_buffer);
	bit_bang_send(command, sizeof command, baud_rate);

	while (!read_buffer_full()); //wait until buffer is full
	
	print_rx_buffer(reset_buffer);
 
	if (os_memcmp(reset_buffer->buf, success, sizeof success) != 0) {
		destroy_rx_buffer(reset_buffer);
		return 1;
	}

	destroy_rx_buffer(reset_buffer);
	return 0;
}

int ICACHE_FLASH_ATTR camera_take_picture()
{
	uint8 command[] = {'\x56', '\x00', '\x36', '\x01', '\x00'};
	uint8 success[] = {'\x76', '\x00', '\x36', '\x00', '\x00'};
	rx_buffer *take_picture_buffer;

	take_picture_buffer = create_rx_buffer(TAKE_PICTURE_RESPONSE_SIZE, NULL);

	if (take_picture_buffer == NULL)
		return -1;

	ets_uart_printf("Taking picture...\n");
	enable_interrupts(gpio_camera_tx, take_picture_buffer); // TODO: Fix the gpio pins!
	bit_bang_send(command, sizeof command, baud_rate);

	while (!read_buffer_full()); //wait until buffer is full

	print_rx_buffer(take_picture_buffer);

	if (os_memcmp(take_picture_buffer->buf, success, sizeof success) != 0) {
		destroy_rx_buffer(take_picture_buffer);
		return 1;
	}

	destroy_rx_buffer(take_picture_buffer);
	return 0;
}

int ICACHE_FLASH_ATTR camera_read_size(uint16 *size_p)
{
	uint8 command[] = {'\x56', '\x00', '\x34', '\x01', '\x00'};
	uint8 success[] = {'\x76', '\x00', '\x34', '\x00', '\x04', '\x00', '\x00'};
	rx_buffer *read_size_buffer;

	read_size_buffer = create_rx_buffer(READ_SIZE_RESPONSE_SIZE, NULL);

	if (read_size_buffer == NULL)
		return -1;

	ets_uart_printf("Reading picture size...\n");
	enable_interrupts(gpio_camera_tx, read_size_buffer);
	bit_bang_send(command, sizeof command, baud_rate);

	while (!read_buffer_full());	// wait until buffer is full

	print_rx_buffer(read_size_buffer);

	if (os_memcmp(read_size_buffer->buf, success, sizeof success) != 0) {
		destroy_rx_buffer(read_size_buffer);
		return 1;
	}

	*size_p = ((read_size_buffer->buf[7]) << 8) | (read_size_buffer->buf[8]);
	destroy_rx_buffer(read_size_buffer);
	return 0;
}

int ICACHE_FLASH_ATTR camera_read_content(uint16 init_addr, uint16 data_len,
						uint16 spacing_int, struct espconn *rep_conn)
{
	uint8 command[] = {'\x56', '\x00', '\x32', '\x0c', '\x00', '\x0a', '\x00', '\x00', \
			   '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'};
	uint8 success[] = {'\x76', '\x00', '\x32', '\x00', '\x00'};
	rx_buffer *read_content_buffer;

	command[8] = (uint8)((init_addr >> 8) & 0xff);
	command[9] = (uint8)(init_addr & 0xff);
	command[12] = (uint8)((data_len >> 8) & 0xff);
	command[13] = (uint8)(data_len & 0xff);
	command[14] = (uint8)((spacing_int >> 8) & 0xff);
	command[15] = (uint8)(spacing_int & 0xff);

	reply_conn = rep_conn;
	read_content_buffer = create_rx_buffer(2 * READ_CONTENT_RESPONSE_SIZE + data_len, camera_picture_recv_done);

	if (read_content_buffer == NULL)
		return -1;

	ets_uart_printf("Reading picture contents...\n");
	enable_interrupts(gpio_camera_tx, read_content_buffer);
	bit_bang_send(command, sizeof command, baud_rate);

	return 0;
}

void ICACHE_FLASH_ATTR camera_stop_pictures()
{
	uint8 command[] = {'\x56', '\x00', '\x36', '\x01', '\x03'};
	bit_bang_send(command, sizeof command, baud_rate);
}

void ICACHE_FLASH_ATTR camera_compression_ratio(uint8 ratio)
{
	uint8 command[] = {'\x56', '\x00', '\x31', '\x05', '\x01', '\x01', '\x12', '\x04', \
			   '\x00'};

	command[8] = ratio;
	bit_bang_send(command, sizeof command, baud_rate);
}

void ICACHE_FLASH_ATTR camera_set_image_size()
{
	// wat
}

void ICACHE_FLASH_ATTR camera_power_saving_on()
{
	uint8 command[] = {'\x56', '\x00', '\x3e', '\x03', '\x00', '\x01', '\x01'};
	bit_bang_send(command, sizeof command, baud_rate);
}

void ICACHE_FLASH_ATTR camera_power_saving_off()
{
	uint8 command[] = {'\x56', '\x00', '\x3e', '\x03', '\x00', '\x01', '\x00'};
	bit_bang_send(command, sizeof command, baud_rate);
}

void ICACHE_FLASH_ATTR camera_set_baud_rate(uint32 baud)
{
	uint8 command[] = {'\x56', '\x00', '\x24', '\x03', '\x01', '\x00', '\x00'};

	switch (baud) {
		case 9600:	command[5] = '\xae';
			   	command[6] = '\xc8';
			   	break;

		case 19200:	command[5] = '\x56';
				command[6] = '\xe4';
				break;

		case 38400:	command[5] = '\x2a';
				command[6] = '\xf2';
				break;

		case 57600:	command[5] = '\x1c';
				command[6] = '\x4c';
				break;

		case 115200:	command[5] = '\x0d';
				command[6] = '\xa6';
				break;

		default:	ets_uart_printf("Camera baud rate not supported.\n");
				return;
	}

	bit_bang_send(command, sizeof command, baud_rate);
	baud_rate = baud;
}

void ICACHE_FLASH_ATTR camera_init(uint32 baud_rate_set, uint8 gpio_camera_rx_set, uint8 gpio_camera_tx_set)
{
	gpio_init();
	baud_rate=baud_rate_set;
	gpio_camera_rx=gpio_camera_rx_set;
	gpio_camera_tx=gpio_camera_tx_set;

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO4_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);

	GPIO_DIS_OUTPUT(gpio_camera_rx);
//	ets_wdt_disable();
	ets_uart_printf("Camera initialized. Rx: %d Tx: %d\n",gpio_camera_rx,gpio_camera_tx);
}
