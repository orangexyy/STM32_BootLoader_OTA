#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "stm32f10x.h"                  // Device header
#include "sys.h" 
//////////////////////////////////////////////////////////////////////////////////	 
//#define USART_RX_SIZE   2048
//#define USART_RX_NUM    256
//#define NUM    10

//typedef struct{
//    uint8_t *p_start;
//    uint8_t *p_end;
//}usart_rx_buffer_ptr;

//typedef struct{
//    uint16_t usart_rx_count;
//    usart_rx_buffer_ptr p_usart_rx_buffer[NUM];
//    usart_rx_buffer_ptr *p_usart_rx_write;
//    usart_rx_buffer_ptr *p_usart_rx_read;
//    usart_rx_buffer_ptr *p_usart_rx_end;
//}usart_rx_buffer_ctr;

//extern uint8_t usart_rx_buffer[USART_RX_SIZE];
//extern usart_rx_buffer_ctr usart1_rx_ctr;


//////////////////////////////////////////////////////////////////////////////////
//串口1初始化		

extern int usart_RxPacket[];
extern uint8_t usart_RxFlag;

void usart1_init(unsigned int bound);

void usart1_rx_buffer_init(void);
void usart1_init(unsigned int bound);
void usart1_dma_init(void);
void usart1_send_byte(uint8_t Byte);
void usart1_send_array(uint8_t *Array, uint16_t Length);
void usart1_send_string(char *String);
uint32_t usart1_pow(uint32_t X, uint32_t Y);		//次方函数  Result = X的Y次方
void usart1_send_number(uint32_t Number, uint8_t Length);		//发送数字


#define USE_USART2_3 0

#if USE_USART2_3
//串口2初始化		

extern unsigned int Usart2_RxPacket[];
extern uint8_t Usart2_RxFlag;

void Usart2_Init(unsigned int bound);
void USART2_SendByte(uint8_t Byte);
void USART2_SendArray(uint8_t *Array, uint16_t Length);
void USART2_SendString(char *String);
uint32_t USART2_Pow(uint32_t X, uint32_t Y);		//次方函数  Result = X的Y次方
void USART2_SendNumber(uint32_t Number, uint8_t Length);		//发送数字

//串口3初始化		

extern unsigned int Usart3_RxPacket[];
extern uint8_t Usart3_RxFlag;

void Usart3_Init(unsigned int bound);
void USART3_SendByte(uint8_t Byte);
void USART3_SendArray(uint8_t *Array, uint16_t Length);
void USART3_SendString(char *String);
uint32_t USART3_Pow(uint32_t X, uint32_t Y);		//次方函数  Result = X的Y次方
void USART3_SendNumber(uint32_t Number, uint8_t Length);		//发送数字


#endif


#endif


