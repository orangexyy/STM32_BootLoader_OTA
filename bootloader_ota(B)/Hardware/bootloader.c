#include "main.h" 
#include "delay.h"
#include "usart3.h"
#include "at24c256.h"
#include "w25q64.h"
#include "flash.h"
#include "bootloader.h" 

set_pc SET_PC;
static uint8_t bootloader_current_event = BOOTLOADER_EVENT_NONE;

static uint8_t xmodem_protocol_current_state = XMODEM_PROTOCOL_STATE_NONE;
XMODEM_PROTOCOL_DATA xmodem_protocol_struct;


void bootloader_branch(void)
{
    if(bootloader_enter_detect(20) == 0)
    {
        if (ota_info_struct.flag == OTA_SET_FLAG)
        {
            usart3_printf("OTA Update For A Block\r\n");
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

void bootloader_deinit_periph(void)
{
    USART_DeInit(USART1);
    USART_DeInit(USART3);
    I2C_DeInit(I2C1);
    GPIO_DeInit(GPIOA);
    GPIO_DeInit(GPIOB);
}

uint8_t bootloader_enter_detect(uint8_t timeout)
{
    usart3_printf("In %d Ms, If Input W, Enter Bootloader Bommand\r\n",timeout*100);
    while (timeout--)
    {
        delay_ms(100);
        if (usart3_rx_buffer[0] == 'W'|| usart3_rx_buffer[0] == 'w')
        {
            return 1;
        }
    }
    return 0;
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

void bootloader_event_detect(void)
{   
    uint16_t rx_data_len = 0;
    uint8_t rx_data[256] = {0};

    if(get_usart3_receive_flag())
    {
        rx_data_len = get_usart3_rx_len();
        get_usart3_rx_buffer(rx_data);
    }
    
   

    if((rx_data_len == 1) && (rx_data[0] == '1'))               //接收到命令[1]
    {
        usart3_printf("Erase A Block\r\n");
		bootloader_current_event = BOOTLOADER_EVENT_ERASE_A;
    }
    else if((rx_data_len == 1) && (rx_data[0] == '2'))          //接收到命令[2]
    {
        usart3_printf("Serial IAP Download For A Block By Xmodem Procotol, Use Bin Format File\r\n");
		bootloader_current_event = BOOTLOADER_EVENT_IAP;
        xmodem_protocol_current_state = XMODEM_PROTOCOL_STATE_START;
    }
    else if((rx_data_len == 1) && (rx_data[0] == '3'))          //接收到命令[3]
    {
        usart3_printf("Set OTA Version\r\n");
		bootloader_current_event = BOOTLOADER_EVENT_SET_OTA_VERSION;
    }
	else if((rx_data_len == 1) && (rx_data[0] == '7'))          //接收到命令[7]
    {
        usart3_printf("System Reset\r\n");
		delay_ms(1000);
		bootloader_current_event = BOOTLOADER_EVENT_SYSTEM_RESET;
    }
    else if((rx_data_len == 133) && (rx_data[0] == 0x01))       //接收到起始帧
    {
        xmodem_protocol_struct.receive_flag = 1;
        bootloader_current_event = BOOTLOADER_EVENT_IAP;
        xmodem_protocol_current_state = XMODEM_PROTOCOL_STATE_RECEIVE_DATA;
    }
    else if((rx_data_len == 1) && (rx_data[0] == 0x04))         //接收到结束帧
    {   
        xmodem_protocol_struct.receive_flag = 1;
        bootloader_current_event = BOOTLOADER_EVENT_IAP;
        xmodem_protocol_current_state = XMODEM_PROTOCOL_STATE_RECEIVE_DATA;
    }
	else
    {

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
            break;
        case BOOTLOADER_EVENT_ERASE_A:
            flash_erase(FLASH_BLOCK_A_START_PAGE,FLASH_BLOCK_A_PAGE_NUM);
            usart3_printf("Erase A Block OK\r\n");
            bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
            break;
        case BOOTLOADER_EVENT_IAP: 
            if(xmodem_protocol_handle() == 1)
            {
                bootloader_current_event = BOOTLOADER_EVENT_NONE;       //清除标志位 
                delay_ms(500);
                NVIC_SystemReset();
            } 
            break;
        case BOOTLOADER_EVENT_SET_OTA_VERSION: 
            bootloader_get_ota_version();
            // bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
            break;
        case BOOTLOADER_EVENT_GET_OTA_VERSION: 
            bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
            break;
        case BOOTLOADER_EVENT_DOWNLOAD_TO_EXTERNAL_FLASH: 
            bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
            break;
        case BOOTLOADER_EVENT_DOWNLOAD_FROM_EXTERNAL_FLASH: 
            bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
            break;
        case BOOTLOADER_EVENT_SYSTEM_RESET: 
            NVIC_SystemReset();
            bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
            break;
        default:
            break;

    }
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
			{
				crcinit = (crcinit << 1) ^ crcipoly;
			}
			else
			{
				crcinit = (crcinit << 1);
			}
        }
        data++;
    }
    return crcinit;
}

uint8_t xmodem_protocol_handle(void)
{
    uint16_t rx_data_len;
    uint8_t rx_data[256];

    switch(xmodem_protocol_current_state)
    {
        case XMODEM_PROTOCOL_STATE_NONE:
            return 0;
        case XMODEM_PROTOCOL_STATE_START:
            flash_erase(FLASH_BLOCK_A_START_PAGE,FLASH_BLOCK_A_PAGE_NUM);
            usart3_printf("Start Serial IAP Download\r\n");
            xmodem_protocol_struct.time = 0;
            xmodem_protocol_struct.receive_buf_num = 0;
            xmodem_protocol_current_state = XMODEM_PROTOCOL_STATE_SEND_C;
            return 0;
        case XMODEM_PROTOCOL_STATE_SEND_C:
            delay_ms(10);
            if(xmodem_protocol_struct.time >= 100)
            {
                usart3_printf("C");
                xmodem_protocol_struct.time = 0;
            }
            xmodem_protocol_struct.time++;
            return 0;
        case XMODEM_PROTOCOL_STATE_RECEIVE_DATA:
            if(xmodem_protocol_struct.receive_flag == 1)
            {
                xmodem_protocol_struct.receive_flag = 0;

                get_usart3_rx_buffer(rx_data);
                rx_data_len = get_usart3_rx_len();

                if((rx_data_len == 133) && (rx_data[0] == 0x01))
                {
                    xmodem_protocol_struct.receive_crc = xmodem_crc16(&rx_data[3], 128);    
                    if(xmodem_protocol_struct.receive_crc == rx_data[131]*256 + rx_data[132])       //crc检验
                    {
                        xmodem_protocol_struct.receive_buf_num++;                                   //接收到的包数（一包128byte）累加
                        memcpy(&update_a_struct.buffer[((xmodem_protocol_struct.receive_buf_num - 1) % (FLASH_PAGE_SIZE/128))*128], &rx_data[3], 128);      //复制到更新A区的buffer里
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
                if((rx_data_len == 1) && (rx_data[0] == 0x04))
                {
                    usart3_printf("\x06");         //接收数据结束，发送ACK
                    if((xmodem_protocol_struct.receive_buf_num % (FLASH_PAGE_SIZE/128)) != 0)           //处理接收到的剩余不满8包（1024byte）写入一页flash的数据
                    {
                        flash_write(FLASH_BLOCK_A_START_ADDR + (xmodem_protocol_struct.receive_buf_num / (FLASH_PAGE_SIZE/128)) * FLASH_PAGE_SIZE, (uint32_t *)update_a_struct.buffer, (xmodem_protocol_struct.receive_buf_num % (FLASH_PAGE_SIZE/128))*128);
                    }
                    xmodem_protocol_current_state = XMODEM_PROTOCOL_STATE_END;
                }
            }
            return 0;
        case XMODEM_PROTOCOL_STATE_END:
            xmodem_protocol_current_state = XMODEM_PROTOCOL_STATE_NONE;
            return 1;
        default:
            return 0;
    }
}





//设置SP指针
//B区起始地址0x08000000
//A区起始地址0x08005000
__asm void SET_SP(uint32_t addr)
{
   MSR MSP, r0
   BX r14 
}

void bootloader_ota_a_block(void)
{
	uint8_t i;
	
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
        bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位
    }
    else
    {
        usart3_printf("len error\r\n");
        bootloader_current_event = BOOTLOADER_EVENT_NONE;		//清除标志位 
    }
}

void bootloader_load_a_block(uint32_t addr)
{
    if((*(uint32_t *)addr >= 0x20000000) && (*(uint32_t *)addr <= 0x20004FFF))
    {
        SET_SP(*(uint32_t *)addr);                      //将向量表中的第一个成员的内容给到SP指针
        SET_PC = (set_pc)*(uint32_t *)(addr+4);         //将向量表中的第二个成员的内容给到PC指针
        SET_PC();
        bootloader_deinit_periph();
        usart3_printf("Go To A Block Success\r\n");
    }
    else 
    {
        usart3_printf("Go To A Block Fail\r\n");
    }
}

void bootloader_get_ota_version(void)
{
    int temp;


    if(usart3_rx_bufferr_len == 9)
    {
        if(sscanf((char *)usart3_rx_buffer,"VER-%d.%d.%d",&temp ,&temp ,&temp) == 3)
        {
            memset(ota_info_struct.version, 0, 26);
            memcpy(ota_info_struct.version, usart3_rx_buffer, 26);
            at24c256_write_ota_data();
            usart3_printf("Set OTA Version Success\r\n");
        }
        else
        {
            usart3_printf("Version Number Format Error\r\n");
        }
    }
    else
    {
        usart3_printf("Version Number Length Error\r\n");
    }

}





