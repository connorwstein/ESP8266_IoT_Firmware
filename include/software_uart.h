#ifndef SOFTWARE_UART_H
#define SOFTWARE_UART_H

void ICACHE_FLASH_ATTR bit_bang_send(const char *, uint16, uint32);
void ICACHE_FLASH_ATTR enable_interrupts(uint8,uint32 *);
void ICACHE_FLASH_ATTR disable_interrupts(uint8);
void ICACHE_FLASH_ATTR read(uint8 gpio_pin, uint16 baud_rate, uint8 *buffer, uint32 buffer_size);

#endif