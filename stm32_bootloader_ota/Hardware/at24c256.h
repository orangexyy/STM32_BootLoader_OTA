#ifndef __AT24C256_H
#define __AT24C256_H

#include "stm32f10x.h"


/* AT24C256 定义 */
#define AT24C256_ADDRESS        0xA0    // 设备地址（A0-A2接地时为0xA0）
#define AT24C256_PAGE_SIZE      64      // 页大小（字节）
#define AT24C256_SIZE           32768   // 总容量（字节）

/* 函数声明 */
void at24c256_init(void);
void at24c256_write_byte(uint16_t addr, uint8_t data);
uint8_t at24c256_read_byte(uint16_t addr);
void at24c256_write_page(uint16_t addr, uint8_t *data, uint8_t len);
void at24c256_read_buffer(uint16_t addr, uint8_t *buffer, uint16_t len);
// void at24c256_Test(void);

#endif


