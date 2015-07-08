#include "c_types.h"
#include "eagle_soc.h"
#include "ets_sys.h"
#include "gpio.h"
#include "osapi.h"
#include "user_interface.h"

#define TOTICKS(x) ((x)*5)>>4
#define BITS_IN_BYTE 8

uint32 baud_rate=38400;
uint32 time_single_bit; //time for a single bit to be transmitted in micro seconds
uint8 reading_instruction_delay=3;
const uint8 num_read_bytes=60;

uint8 buffer[60]={0};
uint16 byte_count=0;

void ICACHE_FLASH_ATTR bit_bang_send(const char *data, uint16 len)
{
	// ETS_GPIO_INTR_DISABLE();
	// gpio_pin_intr_state_set(5,GPIO_PIN_INTR_DISABLE); 
	char byte;
	uint16 i, j;

	for (i = 0; i < len; i++) {
		byte = data[i];
		j = 0;

		/* Start bit */
		GPIO_OUTPUT_SET(4, 0);
		os_delay_us(time_single_bit);

		/* Data bits */
		while (j < 8) {
			/* ADD parentheses here since macro doesn't.
				Precedence is fucked up. */
			GPIO_OUTPUT_SET(4, (byte & 0x1));
			os_delay_us(time_single_bit);
			byte >>= 1;
			j++;
		}

		/* Stop bit */
		GPIO_OUTPUT_SET(4, 1);
		os_delay_us(time_single_bit);
	}

	GPIO_DIS_OUTPUT(4);
	// gpio_pin_intr_state_set(5,GPIO_PIN_INTR_NEGEDGE); 
	// ETS_GPIO_INTR_ENABLE();
}


void bit_bang_read_byte(uint32 intr_mask, void *arg)
{
	int i=0;
	uint8 byte=0;
	//ets_uart_printf("IN READ\n");
	gpio_intr_ack(intr_mask);
	uint32 gpio_status=GPIO_REG_READ(GPIO_STATUS_ADDRESS);	
	
	if(gpio_status==0) return; //Not actual data when gpio_status is 0
	
	os_delay_us(time_single_bit); 
	os_delay_us(time_single_bit/2); //wait until halfway through the first real bit to minimize errors
	
	while(i<BITS_IN_BYTE){
		//loop through next 8 bits, putting each read into a single bit of "byte"
		byte=(byte>>1)|(GPIO_INPUT_GET(5)<<(BITS_IN_BYTE-1)); 
		os_delay_us(time_single_bit-reading_instruction_delay); // 23 rather than 26, because the read above takes some time
		i++;
	}
	//ets_uart_printf("Byte: %c Hex: %08x Byte Count: %d\n",byte,byte,byte_count);
	buffer[byte_count++]=byte;
	if(byte_count>sizeof(buffer)){
		int j=0;
		ets_uart_printf("\n Received Hex \n");
		while(j<=sizeof(buffer)){
			ets_uart_printf(" %02x",buffer[j]);
			j++;
		}
		ets_uart_printf("\n Received Ascii \n");
		j=0;
		while(j<=sizeof(buffer)){
			ets_uart_printf("%c",buffer[j]);
			j++;
		}
		ets_uart_printf("\n");
		byte_count=0;
	}
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
}


void ICACHE_FLASH_ATTR init_done()
{
	char reset[]= {'\x56','\x00','\x26','\x00'};
	char data[10];
	int i;
	time_single_bit= 1000000/baud_rate;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO4_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);
	GPIO_DIS_OUTPUT(5);
	//For some reason sending this data after the interrupts have been attached 
	//causes the interrupts to not work?	
	
	ETS_GPIO_INTR_ATTACH(bit_bang_read_byte, NULL);
	ETS_GPIO_INTR_DISABLE();
	gpio_pin_intr_state_set(5,GPIO_PIN_INTR_NEGEDGE);  
	ETS_GPIO_INTR_ENABLE();
	bit_bang_send(reset,sizeof reset);

}

void ICACHE_FLASH_ATTR user_init()
{
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	system_set_os_print(0);
	ets_wdt_disable();
	gpio_init();
	system_init_done_cb(init_done);
}
