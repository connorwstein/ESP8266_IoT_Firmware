#include "c_types.h"
#include "spi_flash.h"

#include "debug.h"

int ICACHE_FLASH_ATTR read_from_flash(uint32 addr, uint32 *data, uint32 size)
{
	DEBUG("enter read_from_flash");
	SpiFlashOpResult read;

	read = spi_flash_read(addr, data, size);

	if (read != SPI_FLASH_RESULT_OK) {
		ets_uart_printf("SPI flash read failed.\n");
		DEBUG("exit read_from_flash");
		return -1;
	}

	DEBUG("exit read_from_flash");
	return 0;
}

int ICACHE_FLASH_ATTR write_to_flash(uint32 addr, uint32 *data, uint32 size)
{
	DEBUG("enter write_to_flash");
	SpiFlashOpResult erase;
	SpiFlashOpResult write;

	erase = spi_flash_erase_sector(addr >> 12);

	if (erase != SPI_FLASH_RESULT_OK) {
		ets_uart_printf("Failed to erase flash sector.\n");
		DEBUG("exit write_to_flash");
		return -1;
	}

	write = spi_flash_write(addr, data, size);

	if (write != SPI_FLASH_RESULT_OK) {
		ets_uart_printf("Write failed.");
		DEBUG("exit write_to_flash");
		return -1;
	}

	DEBUG("exit write_to_flash");
	return 0;
}
