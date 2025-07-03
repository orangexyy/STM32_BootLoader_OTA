#ifndef __AT24C02_H__
#define __AT24C02_H__

#include "stm32f10x.h"                  // Device header

#define AT24C01		127
#define AT24C02		255
#define AT24C04		511
#define AT24C08		1023
#define AT24C16		2047
#define AT24C32		4095
#define AT24C64	    8191
#define AT24C128	16383
#define AT24C256	32767

//DS1307模块上所使用的是24c32，所以定义EE_TYPE为AT24C32
#define EE_TYPE AT24C256	//需要根据模块具体型号选择
					  
uint8_t at24cxx_read_byte(uint16_t ReadAddr);							//指定地址读取一个字节
void at24cxx_write_byte(uint16_t WriteAddr,uint8_t DataToWrite);		//指定地址写入一个字节
void at24cxx_write_len_byte(uint16_t WriteAddr,uint32_t DataToWrite,uint8_t Len);//指定地址开始写入指定长度的数据
uint32_t at24cxx_read_len_byte(uint16_t ReadAddr,uint8_t Len);					//指定地址开始读取指定长度数据
void at24cxx_write(uint16_t WriteAddr,uint8_t *pBuffer,uint16_t NumToWrite);	//从指定地址开始写入指定长度的数据
void at24cxx_read(uint16_t ReadAddr,uint8_t *pBuffer,uint16_t NumToRead);   	//从指定地址开始读出指定长度的数据

uint8_t at24cxx_check(void);  //检查器件
void at24cxx_init(void); //初始化IIC


#endif
