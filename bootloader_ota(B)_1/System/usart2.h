#ifndef __UASRT2_H
#define __UASRT2_H
#include "stm32f10x.h"

#define USART2_RX_SIZE   256
#define USART2_TX_SIZE   256
#define USART2_RX_MAX    256

extern uint8_t usart2_rx_buffer[USART2_RX_SIZE];
extern uint16_t usart2_rx_len;

uint8_t get_usart2_receive_flag(void);
void get_usart2_rx_buffer(char* buf);
uint16_t get_usart2_rx_len(void);
void clear_usart2_rx_buffer(void);

void usart2_send_byte(uint8_t data);
void usart2_send_string(char *String);
uint32_t usart2_pow(uint32_t X, uint32_t Y);
void usart2_send_number(uint32_t Number, uint8_t Length);
// void usart2_printf(char *format,...);

void usart2_init(unsigned int bound);

#endif
