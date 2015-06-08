#ifndef FLASH_H
#define FLASH_H

#include "c_types.h"

#define FLASH_ALIGN_LEN(n) (((n) + 3) & 0xfffffff0)

int ICACHE_FLASH_ATTR read_from_flash(uint32 addr, uint32 *data, uint32 size);
int ICACHE_FLASH_ATTR write_to_flash(uint32 addr, const uint32 *data, uint32 size);

#endif
