#include "main.h" 
#include "delay.h"
#include "usart3.h"
#include "at24c256.h"
#include "w25q64.h"
#include "flash.h"
#include "bootloader.h" 

set_pc SET_PC;
XMODEM_PROTOCOL_DATA xmodem_protocol_struct;
static uint8_t bootloader_current_event = BOOTLOADER_EVENT_NONE;

uint8_t bootloader_enter_detect(uint8_t timeout)
{
    usart3_printf("In %d Ms, If Input W, Enter Bootloader Command\r\n",timeout*100);
    while (timeout--)
    {
        delay_ms(100);
        if (usart3_rx_buffer[0] == 'W'|| usart3_rx_buffer[0] == 'w')
        {
            return 1;
        }
    }return 0;
}

void bootloader_enter_info_printf(void)
{
    usart3_printf("\r\n");
    usart3_printf("[1]Erase A Block\r\n");
    usart3_printf("[2]Serial IAP Download For A Block\r\n");
    usart3_printf("[3]Set OTA Version\r\n");
    usart3_printf("[4]Get OTA Version\r\n");
    usart3_printf("[5]Download The Program To External FLASH\r\n");
    usart3_printf("[6]Download The Program From External FLASH\r\n");
    usart3_printf("[7]System Reset\r\n");
}

void bootloader_deinit_periph(void)
{
    USART_DeInit(USART1);
    USART_DeInit(USART3);
    I2C_DeInit(I2C1);
    GPIO_DeInit(GPIOA);
    GPIO_DeInit(GPIOB);
}

void bootloader_branch(void)
{
    if(bootloader_enter_detect(20) == 0)
    {
        if (ota_info_struct.flag == OTA_SET_FLAG)
        {
            usart3_printf("OTA Update\r\n");
            bootloader_current_event = BOOTLOADER_EVENT_OTA;
            update_a_struct.data_block_num = 0;      //ota对于0号分区
        }
        else
        {
            usart3_printf("Go To A Block\r\n");
            bootloader_load_a_block(FLASH_BLOCK_A_START_ADDR);
        }
    }
    usart3_printf("Enter Bootloader Command\r\n");
    bootloader_enter_info_printf();
    
}


uint16_t xmodem_crc16(uint8_t *data, uint16_t datalen)
{
    uint8_t i;
    uint16_t crcinit = 0x0000;
    uint16_t crcipoly = 0x1021;

    while(datalen--)
    {
        crcinit = (*data << 8) ^ crcinit;
        for (i=0; i<8; i++)
        {
            if(crcinit&0x8000)
				crcinit = (crcinit << 1) ^ crcipoly;
			else
				crcinit = (crcinit << 1);
        }data++;
    }return crcinit;
}


//设置SP指针
//B区起始地址0x08000000
//A区起始地址0x08005000
__asm void SET_SP(uint32_t addr)
{
   MSR MSP, r0
   BX r14 
}

void bootloader_load_a_block(uint32_t addr)
{
    if((*(uint32_t *)addr >= 0x20000000) && (*(uint32_t *)addr <= 0x20004FFF))
    {
        SET_SP(*(uint32_t *)addr);                      //将向量表中的第一个成员的内容给到SP指针
        SET_PC = (set_pc)*(uint32_t *)(addr+4);         //将向量表中的第二个成员的内容给到PC指针
        bootloader_deinit_periph();
        SET_PC();
        usart3_printf("Go To A Block Success\r\n");
    }
    else 
    {
        usart3_printf("Go To A Block Fail\r\n");
    }
}

void bootloader_ota_a_block(void)
{
    uint8_t i;

    usart3_printf("Start OTA Update\r\n");
    usart3_printf("len %d byte\r\n",ota_info_struct.app_data_size[update_a_struct.data_block_num]);
    if (ota_info_struct.app_data_size[update_a_struct.data_block_num] % 4 == 0)	//判断字节长度是否正确
    {
        flash_erase(FLASH_BLOCK_A_START_PAGE,FLASH_BLOCK_A_PAGE_NUM);
        for (i = 0; i<ota_info_struct.app_data_size[update_a_struct.data_block_num]/FLASH_PAGE_SIZE; i++)
        {
            w25q64_read_data(i*1024 + update_a_struct.data_block_num*64*1024, update_a_struct.buffer, FLASH_PAGE_SIZE);
            flash_write(FLASH_BLOCK_A_START_ADDR + i*FLASH_PAGE_SIZE, (uint32_t *)update_a_struct.buffer, FLASH_PAGE_SIZE);
        }
        if(ota_info_struct.app_data_size[update_a_struct.data_block_num] % 1024 != 0)
        {
            w25q64_read_data(i*1024 + update_a_struct.data_block_num*64*1024, update_a_struct.buffer, ota_info_struct.app_data_size[update_a_struct.data_block_num] % 1024);
            flash_write(FLASH_BLOCK_A_START_ADDR + i*FLASH_PAGE_SIZE, (uint32_t *)update_a_struct.buffer, ota_info_struct.app_data_size[update_a_struct.data_block_num] % 1024);
        }
        if (update_a_struct.data_block_num == 0)
        {
            ota_info_struct.flag = 0;
            at24c256_write_ota_data();
        }
        NVIC_SystemReset();		//系统复位 
    }
    else
    {
        usart3_printf("len error\r\n");
    }
}

void bootloader_erase_a_block(void)
{
    usart3_printf("Erase A Block\r\n");
    flash_erase(FLASH_BLOCK_A_START_PAGE,FLASH_BLOCK_A_PAGE_NUM);
    usart3_printf("Erase A Block OK\r\n");
}

void bootloader_iap_start(void)
{
    static uint8_t iap_start_flag = 0;

    if (iap_start_flag == 0)
    {
        iap_start_flag = 1;
        usart3_printf("Serial IAP Download For A Block By Xmodem Procotol, Use Bin Format File\r\n");
        flash_erase(FLASH_BLOCK_A_START_PAGE,FLASH_BLOCK_A_PAGE_NUM);
        usart3_printf("Start Serial IAP Download\r\n");
        xmodem_protocol_struct.time = 0;
        xmodem_protocol_struct.receive_buf_num = 0;
    }
    delay_ms(10);
    if(xmodem_protocol_struct.time >= 100)
    {
        usart3_printf("C");
        xmodem_protocol_struct.time = 0;
    }
    xmodem_protocol_struct.time++;
}

void bootloader_iap_receive(void)
{
    xmodem_protocol_struct.receive_crc = xmodem_crc16(&usart3_rx_buffer[3], 128);    
    if(xmodem_protocol_struct.receive_crc == usart3_rx_buffer[131]*256 + usart3_rx_buffer[132])       //crc检验
    {
        xmodem_protocol_struct.receive_buf_num++;                                   //接收到的包数（一包128byte）累加
        memcpy(&update_a_struct.buffer[((xmodem_protocol_struct.receive_buf_num - 1) % (FLASH_PAGE_SIZE/128))*128], &usart3_rx_buffer[3], 128);      //复制到更新A区的buffer里
        if((xmodem_protocol_struct.receive_buf_num % (FLASH_PAGE_SIZE/128)) == 0)       //每接收到8包（1024byte）写入一页flash
        {
            flash_write(FLASH_BLOCK_A_START_ADDR + ((xmodem_protocol_struct.receive_buf_num / (FLASH_PAGE_SIZE/128)) - 1) * FLASH_PAGE_SIZE, (uint32_t *)update_a_struct.buffer, FLASH_PAGE_SIZE);
        }
        usart3_printf("\x06");     //检验通过，发送ACK
    }
    else
    {
        usart3_printf("\x15");     //检验失败，发送NACK
    }
}

void bootloader_iap_end(void)
{
    usart3_printf("\x06");         //接收数据结束，发送ACK
    if((xmodem_protocol_struct.receive_buf_num % (FLASH_PAGE_SIZE/128)) != 0)           //处理接收到的剩余不满8包（1024byte）写入一页flash的数据
    {
        flash_write(FLASH_BLOCK_A_START_ADDR + (xmodem_protocol_struct.receive_buf_num / (FLASH_PAGE_SIZE/128)) * FLASH_PAGE_SIZE, (uint32_t *)update_a_struct.buffer, (xmodem_protocol_struct.receive_buf_num % (FLASH_PAGE_SIZE/128))*128);
    }
    delay_ms(100);
    NVIC_SystemReset();
}

uint8_t bootloader_set_ota_version(void)
{
    static uint8_t flag = 0;
    int temp;

    if(flag == 0)
    {
        usart3_printf("Set OTA Version\r\n");
        usart3_printf("Please Input OTA Version (Format:VER-X.X.X)\r\n");
        flag = 1;
    }
    
    if(usart3_rx_buffer_len == 9)
    {
        if(sscanf((char *)usart3_rx_buffer,"VER-%d.%d.%d", &temp, &temp, &temp) == 3)
        {
            memset(ota_info_struct.version, 0, 16);
            memcpy(ota_info_struct.version, usart3_rx_buffer, 9);
            at24c256_write_ota_data();
            usart3_printf("Set OTA Version Success : %s\r\n",ota_info_struct.version);
            delay_ms(1000);
            bootloader_enter_info_printf();
            return 1; 
        }
        else
        {
            usart3_printf("OTA Version Format Error : %s\r\n",usart3_rx_buffer);
            usart3_printf("Please Input OTA Version Again (Format:VER-X.X.X)\r\n");
            memset(usart3_rx_buffer, 0, usart3_rx_buffer_len);
            usart3_rx_buffer_len = 0;
            return 0;
        }
    }
	return 0;
}

void bootloader_get_ota_version(void)
{
    usart3_printf("Get OTA Version\r\n");
    at24c256_read_ota_data();
    usart3_printf("OTA Version Is : %s\r\n",ota_info_struct.version);
    delay_ms(1000);
    bootloader_enter_info_printf();
}

void bootloader_system_reset(void)
{
    usart3_printf("System Will Reset\r\n");
    delay_ms(1000);
    NVIC_SystemReset();
}


void bootloader_event_detect(void)
{   
    if(get_usart3_receive_flag())
    {
        if((usart3_rx_buffer_len == 1) && (usart3_rx_buffer[0] == '1'))
        {
            bootloader_current_event = BOOTLOADER_EVENT_ERASE_A;
        }
        else if((usart3_rx_buffer_len == 1) && (usart3_rx_buffer[0] == '2'))
        {
            bootloader_current_event = BOOTLOADER_EVENT_IAP_START;
        }
        else if((usart3_rx_buffer_len == 1) && (usart3_rx_buffer[0] == '3'))
        {
            bootloader_current_event = BOOTLOADER_EVENT_SET_OTA_VERSION;
        }
        else if((usart3_rx_buffer_len == 1) && (usart3_rx_buffer[0] == '4'))
        {
            bootloader_current_event = BOOTLOADER_EVENT_GET_OTA_VERSION;
        }
        else if((usart3_rx_buffer_len == 1) && (usart3_rx_buffer[0] == '7'))
        {
            bootloader_current_event = BOOTLOADER_EVENT_SYSTEM_RESET;
        }
        else if((usart3_rx_buffer_len == 133) && (usart3_rx_buffer[0] == 0x01))
        {
            bootloader_current_event = BOOTLOADER_EVENT_IAP_RECEIVE_DATA;
        }
        else if((usart3_rx_buffer_len == 1) && (usart3_rx_buffer[0] == 0x04))
        {
            bootloader_current_event = BOOTLOADER_EVENT_IAP_END;
        }
    }
}

void bootloader_event_handle(void)
{ 
    switch(bootloader_current_event)
    {
        case BOOTLOADER_EVENT_NONE:
            break;
        case BOOTLOADER_EVENT_OTA:
            bootloader_ota_a_block();
            bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
            break;
        case BOOTLOADER_EVENT_ERASE_A:
            bootloader_erase_a_block();
            bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
            break;
        case BOOTLOADER_EVENT_IAP_START: 
            bootloader_iap_start();
            break;
        case BOOTLOADER_EVENT_IAP_RECEIVE_DATA: 
            bootloader_iap_receive();
            bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
            break;
        case BOOTLOADER_EVENT_IAP_END: 
            bootloader_iap_end();
            bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
            break;  
        case BOOTLOADER_EVENT_SET_OTA_VERSION: 
            if(bootloader_set_ota_version() == 1)
            {
                bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
            }
            break;
        case BOOTLOADER_EVENT_GET_OTA_VERSION:
            bootloader_get_ota_version();
            bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
            break;
        case BOOTLOADER_EVENT_DOWNLOAD_TO_EXTERNAL_FLASH: 
            bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
            break;
        case BOOTLOADER_EVENT_DOWNLOAD_FROM_EXTERNAL_FLASH: 
            bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
            break;
        case BOOTLOADER_EVENT_SYSTEM_RESET:
            bootloader_system_reset();
            bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
            break;
        default:
            break;

    }
}






