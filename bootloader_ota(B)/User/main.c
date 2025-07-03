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
#include "bootloader.h" 

OTA_INFO_DATA ota_info_struct;
UPDATE_A_DATA update_a_struct;

void usart3_test(void);
void w25q64_test(void);
void at24c256_test(void);
void flash_test(void);

int main(void)
{
	uint8_t i;
	
	delay_init();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	
	usart1_init(115200);
	usart3_init(115200);
	w25q64_init();
	at24c256_init();
	ota_info_struct.ota_flag = 0x11223347;
	for(i=0; i<11; i++)
	{
		ota_info_struct.app_data_size[i] = i;
	}
	at24c256_write_ota_data();
	at24c256_read_ota_data();
		
	usart3_printf("ota_flag:%x\r\n",ota_info_struct.ota_flag);
	for(i=0; i<11; i++)
	{
		usart3_printf("%x\r\n",ota_info_struct.app_data_size[i]);
	}
	
	bootloader_branch();
	
	usart3_printf("\r\n start \r\n");
	

    while(1)
    {

    }	
}

void usart3_test(void)
{
	char rx_buffer[512];

	usart3_printf("usart3 test \r\n");
	if( get_usart3_receive_flag() )
	{
		get_usart3_rx_buffer(rx_buffer);
		usart3_printf("receive %d data\n data : %s\n", get_usart3_rx_len(),rx_buffer);//串口回传
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

void flash_test(void)
{
	uint32_t i;
	uint32_t wdata[1024];

	usart3_printf("flash test \r\n");

	for ( i = 0; i < 1024; i++)
	{
		wdata[i] = 0x12345678;
	}
	flash_erase(60,4);
	flash_write(60*1024 + 0x08000000, wdata, 1024*4);
	for ( i = 0; i < 1024; i++)
	{
		usart3_printf("%x\r\n",*(uint32_t *)((60*1024 + 0x08000000) + (i*4)));
	}	
}




