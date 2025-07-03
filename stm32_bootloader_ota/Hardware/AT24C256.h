#ifndef __AT24C256_H
#define __AT24C256_H

#include "stm32f10x.h"


/* AT24C256 定义 */
#define AT24C256_ADDRESS        0xA0    // 设备地址（A0-A2接地时为0xA0）
#define AT24C256_PAGE_SIZE      64      // 页大小（字节）
#define AT24C256_SIZE           32768   // 总容量（字节）

/* 函数声明 */
void I2C_Configuration(void);
void AT24C256_WriteByte(uint16_t addr, uint8_t data);
uint8_t AT24C256_ReadByte(uint16_t addr);
void AT24C256_WritePage(uint16_t addr, uint8_t *data, uint8_t len);
void AT24C256_ReadBuffer(uint16_t addr, uint8_t *buffer, uint16_t len);
void AT24C256_Test(void);

#endif


