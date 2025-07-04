#ifndef __UASRT3_H
#define __UASRT3_H
#include "stm32f10x.h"

#define USART_RX_SIZE   2048
#define USART_TX_SIZE   512
#define USART_RX_MAX    512
#define NUM    10

// typedef struct{
//     uint8_t *p_start;
//     uint8_t *p_end;
// }usart_rx_buffer_ptr;

// typedef struct{
//     uint16_t usart_rx_count;
//     usart_rx_buffer_ptr p_usart_rx_buffer[NUM];
//     usart_rx_buffer_ptr *p_usart_rx_write;
//     usart_rx_buffer_ptr *p_usart_rx_read;
//     usart_rx_buffer_ptr *p_usart_rx_end;
// }usart_rx_buffer_ctr;

// extern uint8_t usart3_rx_buffer[USART_RX_SIZE];
// extern usart_rx_buffer_ctr usart3_rx_ctr;

uint8_t get_usart3_receive_flag(void);
void get_usart3_rx_buffer(char* buf);
uint16_t get_usart3_rx_len(void);
void clear_usart3_rx_buffer(void);

void usart3_send_byte_blocking(uint8_t data);
void usart3_printf(char* fmt,...);

void usart3_init(unsigned int bound);
void usart3_rx_buffer_Init(void);

#endif
