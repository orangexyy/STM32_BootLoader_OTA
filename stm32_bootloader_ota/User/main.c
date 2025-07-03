#include "stm32f10x.h"                  // Device header
#include "sys.h"
#include "main.h" 
#include "delay.h"
#include "led.h"
#include "key.h"
#include "oled.h"
#include "usart.h"
#include "usart3.h"
#include "at24c02.h"
#include "at24c256.h"
#include "w25q64.h"
#include "flash.h"

uint16_t i;
char rx_buffer[512];

void w25q64_test(void);
void at24c256_test(void);

int main(void)
{
	delay_init();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	
	usart1_init(115200);

	usart3_init(115200);
	w25q64_init();
	w25q64_sector_erase_64k(0);
	// w25q64_test();
	at24c256_init();
	// at24c256_test();
	
	usart3_printf("\r\n start \r\n");

    while(1)
    {
        if( get_usart3_receive_flag() )
        {
			get_usart3_rx_buffer(rx_buffer);
            usart3_printf("receive %d data\n data : %s\n", get_usart3_rx_len(),rx_buffer);//串口回传
			// for ( i = 0; i < get_usart3_rx_len(); i++)
			// {
			// 	printf("%c",rx_buffer[i]);
			// }
			// printf("\r\n\r\n");
			// if(usart3_rx_ctr.p_usart_rx_read != usart1_rx_ctr.p_usart_rx_write)
			// {
			// 	printf("receive %d byte data\r\n",usart1_rx_ctr.p_usart_rx_read->p_end - usart1_rx_ctr.p_usart_rx_read->p_start + 1);
			// 	for ( i = 0; i < usart1_rx_ctr.p_usart_rx_read->p_end - usart1_rx_ctr.p_usart_rx_read->p_start + 1; i++)
			// 	{
			// 		printf("%c",usart1_rx_ctr.p_usart_rx_read->p_start[i]);
			// 	}
			// 	printf("\r\n\r\n");
			// 	usart1_rx_ctr.p_usart_rx_read ++;
			// 	if(usart1_rx_ctr.p_usart_rx_read == usart1_rx_ctr.p_usart_rx_end)
			// 	{
			// 		usart1_rx_ctr.p_usart_rx_read = &usart1_rx_ctr.p_usart_rx_buffer[0];
			// 	}
			// }	
        }
    }	
}

void w25q64_test(void)
{
	uint16_t i,j;

	uint8_t wdata[256];
	uint8_t rdata[256];

	usart3_printf("w25q64 test \r\n");
	w25q64_sector_erase_64k(0);
	for ( i = 0; i < 256; i++)
	{
		for ( j = 0; j < 256; j++)
		{
			wdata[j] = i;
		}
		w25q64_write_page(i,wdata,256);
	}
	delay_ms(50);
	for ( i = 0; i < 256; i++)
	{
		w25q64_read_data(i*256,rdata,256);
		for ( j = 0; j < 256; j++)
		{
			usart3_printf("addr : %d = %x\r\n",i*256+j,rdata[j]);
		}
	}	
}

/* 测试函数 */
void at24c256_test(void)
{
    uint8_t test_data[64] = {0};
    uint8_t read_data[64] = {0};
    uint16_t i;
    
    /* 准备测试数据 */
    for (i = 0; i < 64; i++) {
        test_data[i] = i+2;
    }
    
    /* 页写入测试（地址0x1000） */
    at24c256_write_page(0x1000, test_data, 64);
    
    /* 读取验证 */
    at24c256_read_buffer(0x1000, read_data, 64);
    
    /* 验证结果 */
    for (i = 0; i < 64; i++) {
        if (read_data[i] != test_data[i]) {
			usart3_printf("\r\n AT24C256 init error \r\n");
			break;
        }
		else
		{
			usart3_printf("\r\n AT24C256 init success \r\n");
			for (i = 0; i < 64; i++) 
			{
				usart3_printf("\r\n test_data %d : %x\r\n",i,test_data[i]);
			}
			
		}
    }
	usart3_printf("\r\n AT24C256 init ok \r\n");
}




