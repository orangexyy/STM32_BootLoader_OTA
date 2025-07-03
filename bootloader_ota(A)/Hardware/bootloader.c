#include "main.h" 
#include "delay.h"
#include "usart3.h"
#include "at24c256.h"
#include "w25q64.h"
#include "flash.h"
#include "bootloader.h" 

set_pc SET_PC;

void bootloader_branch(void)
{
    if (ota_info_struct.ota_flag == OTA_SET_FLAG)
    {
        usart3_printf("\r\n OTA update \r\n");
    }
    else
    {
        usart3_printf("\r\n go to A block \r\n");
        bootloader_load_a_block(FLASH_BLOCK_A_START_ADDR);
    }
}

void bootloader_deinit_periph(void)
{
    USART_DeInit(USART1);
    USART_DeInit(USART3);
    I2C_DeInit(I2C1);
    GPIO_DeInit(GPIOA);
    GPIO_DeInit(GPIOB);
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
    }
}





