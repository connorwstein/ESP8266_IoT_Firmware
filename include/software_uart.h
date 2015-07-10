#ifndef SOFTWARE_UART_H
#define SOFTWARE_UART_H

struct rx_buffer;

typedef void (*rx_buffer_recv_cb)(struct rx_buffer *);

typedef struct rx_buffer{
	uint16 size;
	uint8 *buf;
	rx_buffer_recv_cb recv_cb;
} rx_buffer;

void ICACHE_FLASH_ATTR bit_bang_send(const char *, uint16, uint32);
void ICACHE_FLASH_ATTR enable_interrupts(uint8,rx_buffer*);
void ICACHE_FLASH_ATTR disable_interrupts(uint8);
void ICACHE_FLASH_ATTR read(uint8 gpio_pin, uint16 baud_rate, uint8 *buffer, uint32 buffer_size);
bool ICACHE_FLASH_ATTR read_buffer_full(void);
void ICACHE_FLASH_ATTR flush_read_buffer(void);

rx_buffer* ICACHE_FLASH_ATTR create_rx_buffer(uint16 response_size, rx_buffer_recv_cb recv_cb);
void ICACHE_FLASH_ATTR destroy_rx_buffer(rx_buffer* buffer_to_kill);

#endif
