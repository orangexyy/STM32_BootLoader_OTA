#ifndef __FLASH_H
#define __FLASH_H

#include "sys.h"
#include "stm32f10x.h"                  // Device header

void flash_erase(uint16_t start_page, uint16_t page_num);
void flash_write(uint32_t start_addr, uint32_t *data, uint32_t len);

#endif


