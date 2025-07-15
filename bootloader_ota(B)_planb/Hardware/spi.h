#ifndef __SPI_H
#define __SPI_H

#include "stm32f10x.h"                  // Device header

void spi_init(void);
void spi_start(void);
void spi_stop(void);
uint8_t spi_write_read_byte(uint8_t byte);

#endif
