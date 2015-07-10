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
uint16 byte_count=0;
rx_buffer *buffer=NULL;

void ICACHE_FLASH_ATTR bit_bang_send(const char *data, uint16 len, uint32 baud_rate_set)
{
	gpio_pin_intr_state_set(5,GPIO_PIN_INTR_DISABLE); 

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
		}

		/* Stop bit */
		GPIO_OUTPUT_SET(4, 1);
		os_delay_us(26);
	}

	gpio_pin_intr_state_set(5,GPIO_PIN_INTR_NEGEDGE); 
	
}
void bit_bang_read_byte(uint32 intr_mask, void* arg){
	int clock=NOW();
	int i=0;
	uint8 byte=0;
	//ets_uart_printf("INREAD\n");
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

	if (byte_count == buffer->size) {
		disable_interrupts(5);
		ets_uart_printf("Finished reading data\n");

		if (buffer->recv_cb != NULL)
			buffer->recv_cb(buffer);
	}

	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
}

void ICACHE_FLASH_ATTR enable_interrupts(uint8 gpio_pin, rx_buffer* buffer_set){
	buffer=buffer_set;
	byte_count = 0;

	ets_uart_printf("buffer size in enable interrupts: %d\n", buffer->size);
	//ets_uart_printf("Enabling interrupts \n");
	ETS_GPIO_INTR_ATTACH(bit_bang_read_byte,NULL);
	ETS_GPIO_INTR_DISABLE(); 
	ETS_GPIO_INTR_ENABLE();
	//ets_uart_printf("After enabled interrupts\n");
}

void ICACHE_FLASH_ATTR disable_interrupts(uint8 gpio_pin){
	gpio_pin_intr_state_set(gpio_pin,GPIO_PIN_INTR_DISABLE);  
	ETS_GPIO_INTR_DISABLE();
}

bool ICACHE_FLASH_ATTR read_buffer_full(void){
	return byte_count<buffer->size? false : true;
}

void ICACHE_FLASH_ATTR flush_read_buffer(void){
	byte_count=0;
	os_memset(buffer->buf,0,buffer->size);
	buffer=NULL;
}

rx_buffer* ICACHE_FLASH_ATTR create_rx_buffer(uint16 response_size, rx_buffer_recv_cb recv_cb)
{
	uint8 *buffer;
	rx_buffer *buffer_struct;

	buffer = (uint8 *)os_zalloc(response_size);

	if (buffer == NULL)
		return NULL;

	buffer_struct = (rx_buffer *)os_zalloc(sizeof (rx_buffer));

	if (buffer_struct == NULL) {
		os_free(buffer);
		return NULL;
	}

	buffer_struct->size=response_size;
	buffer_struct->buf=buffer;
	buffer_struct->recv_cb = recv_cb;
	return buffer_struct;
}

void ICACHE_FLASH_ATTR destroy_rx_buffer(rx_buffer* buffer_to_kill)
{
	flush_read_buffer(); 

	if (buffer_to_kill == NULL)
		return;

	os_free(buffer_to_kill->buf);
	os_free(buffer_to_kill);
}
