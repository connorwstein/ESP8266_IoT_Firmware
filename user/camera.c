#include "c_types.h"
#include "eagle_soc.h"
#include "gpio.h"
#include "software_uart.h"
#include "ets_sys.h"

#define GPIO_TX_PIN 4
#define GPIO_RX_PIN 5

static uint32 baud_rate = 38400;

void ICACHE_FLASH_ATTR camera_reset()
{
	uint8 command[] = {'\x56', '\x00', '\x26', '\x00'};
	int reset_response=60;
	ets_uart_printf("Resetting camera...\n");
	enable_interrupts(5,&reset_response);
	bit_bang_send(command,sizeof command,baud_rate);
}

void ICACHE_FLASH_ATTR camera_take_picture()
{
	uint8 command[] = {'\x56', '\x00', '\x36', '\x01', '\x00'};
	bit_bang_send(command, sizeof command, baud_rate);
}

void ICACHE_FLASH_ATTR camera_read_size()
{
	uint8 command[] = {'\x56', '\x00', '\x34', '\x01', '\x00'};
	bit_bang_send(command, sizeof command, baud_rate);
}

void ICACHE_FLASH_ATTR camera_read_content(uint16 init_addr, uint16 data_len, uint16 spacing_int)
{
	uint8 command[] = {'\x56', '\x00', '\x32', '\x0c', '\x00', '\x0a', '\x00', '\x00', \
			   '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'};

	command[8] = (uint8)(init_addr & 0xff);
	command[9] = (uint8)((init_addr >> 8) & 0xff);
	command[12] = (uint8)(data_len & 0xff);
	command[13] = (uint8)((data_len >> 8) & 0xff);
	command[14] = (uint8)(spacing_int & 0xff);
	command[15] = (uint8)((spacing_int >> 8) & 0xff);

	bit_bang_send(command, sizeof command, baud_rate);
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

void ICACHE_FLASH_ATTR camera_init(uint32 baud_rate_set)
{
	gpio_init();
	baud_rate=baud_rate_set;
	ets_uart_printf("CAMERA INIT\n");
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO4_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);

	GPIO_DIS_OUTPUT(GPIO_RX_PIN);
	//GPIO_DIS_OUTPUT(GPIO_TX_PIN);
	ets_wdt_disable();
}
