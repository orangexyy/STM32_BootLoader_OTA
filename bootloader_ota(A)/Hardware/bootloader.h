#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "stm32f10x.h"
#include "sys.h"       

typedef void (*set_pc)(void);

void bootloader_branch(void);
void bootloader_deinit_periph(void);
__asm void SET_SP(uint32_t addr);
void bootloader_load_a_block(uint32_t addr);

#endif



