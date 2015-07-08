#include "c_types.h"
#include "eagle_soc.h"
#include "ets_sys.h"
#include "gpio.h"
#include "osapi.h"
#include "user_interface.h"

#define TOTICKS(x) ((x)*5)>>4

uint32 baud = 38400;
uint32 delay = 25;

uint8 buffer_raw[400];
//TEST COMMIT IN PRIVATE REPO

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


void bit_bang_read_byte(uint32 intr_mask, void *arg)
{
	uint32 clock;
	int i=0;
	gpio_intr_ack(intr_mask);
	uint32 gpio_status=GPIO_REG_READ(GPIO_STATUS_ADDRESS);	
	// uint8 buffer[500];
	clock=NOW();
	while((NOW()-clock)<TOTICKS(400)&&i<500){
		buffer_raw[i++]=GPIO_INPUT_GET(5);
	}
	ets_uart_printf("Intr Mask: %08x Gpio Status: %08x\n",intr_mask, gpio_status);
	ets_uart_printf("i: %d DATA: ",i);
	int k;
	for(k=0;k<i;k++){
		ets_uart_printf("%d",buffer[k]);
	}
	ets_uart_printf("\n");
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
}


void ICACHE_FLASH_ATTR init_done()
{
	char *string = "HELLO WORLD!\r\n";
	char data[10];
	int i;

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO4_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);
	GPIO_DIS_OUTPUT(5);
	//For some reason sending this data after the interrupts have been attached 
	//causes the interrupts to not work?
	for (i = 0; i < 10; i++) {
		ets_uart_printf("Sending byte\n");
		bit_bang_send(string, strlen(string));
		os_delay_us(100000);
	}
	
	ETS_GPIO_INTR_ATTACH(bit_bang_read_byte, NULL);
	ETS_GPIO_INTR_DISABLE();
	gpio_pin_intr_state_set(5,GPIO_PIN_INTR_NEGEDGE);  
	ETS_GPIO_INTR_ENABLE();

}

void ICACHE_FLASH_ATTR user_init()
{
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	system_set_os_print(0);
	ets_wdt_disable();

	gpio_init();
	system_init_done_cb(init_done);
}
