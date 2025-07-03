#ifndef __W25Q64_H
#define __W25Q64_H

#include "stm32f10x.h"                  // Device header

void w25q64_init(void);
void w25q64_read_id(uint8_t *mid, uint16_t *did);
void w25q64_write_page(uint32_t addr, uint8_t *buf, uint16_t len);
void w25q64_sector_erase_64k(uint32_t addr);
void w25q64_read_data(uint32_t addr, uint8_t *buf, uint32_t len);

#endif
