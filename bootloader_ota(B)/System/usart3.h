#ifndef __UASRT3_H
#define __UASRT3_H
#include "stm32f10x.h"

#define USART_RX_SIZE   256
#define USART_TX_SIZE   256
#define USART_RX_MAX    256

extern uint8_t usart3_rx_buffer[USART_RX_SIZE];
extern uint16_t usart3_rx_buffer_len;

uint8_t get_usart3_receive_flag(void);
void get_usart3_rx_buffer(char* buf);
uint16_t get_usart3_rx_len(void);
void clear_usart3_rx_buffer(void);

void usart3_send_byte(uint8_t data);
void usart3_send_string(char *String);
void usart3_printf(char *format,...);

void usart3_init(unsigned int bound);
void usart3_rx_buffer_Init(void);

#endif
