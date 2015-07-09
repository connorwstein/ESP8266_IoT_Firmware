#include "c_types.h"
#include "eagle_soc.h"
#include "ets_sys.h"
#include "gpio.h"
#include "osapi.h"
#include "mem.h"

#include "software_uart.h"

#define TOTICKS(x) ((x)*5)>>4
#define BITS_IN_BYTE 8

uint32 baud_rate=38400; //default baudrate
static uint16 byte_count=0;
rx_buffer *buffer=NULL;

void ICACHE_FLASH_ATTR bit_bang_send(const char *data, uint16 len, uint32 baud_rate_set)
{
	gpio_pin_intr_state_set(5,GPIO_PIN_INTR_DISABLE); 

	ets_uart_printf("SENDING at %d, each bit %d\n",baud_rate,1000000/baud_rate);
	baud_rate=baud_rate_set;
	char byte;
	uint16 i, j;

	for (i = 0; i < len; i++) {
		byte = data[i];
		j = 0;

		/* Start bit */
		GPIO_OUTPUT_SET(4, 0);
		os_delay_us(26);

		/* Data bits */
		while (j < 8) {
			/* ADD parentheses here since macro doesn't.
				Precedence is fucked up. */
			GPIO_OUTPUT_SET(4, (byte & 0x1));
			os_delay_us(25);
			byte >>= 1;
			j++;
			//ets_uart_printf("Send byte\n");
		}

		/* Stop bit */
		GPIO_OUTPUT_SET(4, 1);
		os_delay_us(26);
	}

	gpio_pin_intr_state_set(5,GPIO_PIN_INTR_NEGEDGE); 
	//ets_uart_printf("Done sending\n");
}
void bit_bang_read_byte(uint32 intr_mask, void* arg){
	int clock=NOW();
	int i=0;
	uint8 byte=0;

	gpio_intr_ack(intr_mask);
	uint32 gpio_status=GPIO_REG_READ(GPIO_STATUS_ADDRESS);	
	if(gpio_status==0) return; //Not actual data when gpio_status is 0
	while((NOW()-clock)<TOTICKS(26+13)); //wait until middle of first bo
	clock=NOW();
	
	while(i<BITS_IN_BYTE){
		//loop through next 8 bits, putting each read into a single bit of "byte"
		byte=(byte>>1)|(GPIO_INPUT_GET(5)<<(BITS_IN_BYTE-1)); 
		while((NOW()-clock)<TOTICKS(26)); // 23 rather than 26, because the read above takes some time
		clock=NOW();
		i++;
	}
	
	buffer->buf[byte_count++]=byte;
	if(byte_count>=buffer->size){
		int j=0;
		ets_uart_printf("\n Received Hex \n",gpio_status);
		while(j<buffer->size){
			ets_uart_printf(" %02x",buffer->buf[j]);
			j++;
		}
		ets_uart_printf("\n Received Ascii \n");
		j=0;
		while(j<buffer->size){
			ets_uart_printf(" %c",buffer->buf[j]);
			j++;
		}
		ets_uart_printf("\n");
		byte_count=0;
		os_memset(buffer->buf,0,buffer->size);
	}
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
}
void ICACHE_FLASH_ATTR enable_interrupts(uint8 gpio_pin, rx_buffer* buffer_set){
	buffer=buffer_set;
	ETS_GPIO_INTR_ATTACH(bit_bang_read_byte,NULL);
	ETS_GPIO_INTR_DISABLE(); 
	ETS_GPIO_INTR_ENABLE();
}

void ICACHE_FLASH_ATTR disable_interrupts(uint8 gpio_pin){
	gpio_pin_intr_state_set(gpio_pin,GPIO_PIN_INTR_DISABLE);  
	ETS_GPIO_INTR_DISABLE();
}

void ICACHE_FLASH_ATTR read_gpio(uint8 gpio_pin, uint16 baud_rate, uint8 *buffer, uint32 buffer_size){
	//assume interrupts have been enabled

}




