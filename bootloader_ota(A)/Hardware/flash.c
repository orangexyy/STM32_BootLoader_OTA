#include "flash.h"
#include "delay.h"

/**
  * 函    数：flash_erase()
                f103c8t6    闪存共64页,闪存页大小为1KB (1024), 擦除以页大小为单位擦除
  * 参    数：start_addr    起始地址    
  * 参    数：page_num       页数
  * 返 回 值：无
  */
void flash_erase(uint16_t start_addr, uint16_t page_num)
{
    uint16_t i;
    FLASH_Unlock();
    for( i=0; i<page_num; i++)
    {
        FLASH_ErasePage((0x08000000 + start_addr * 1024) + (1024 * i));
    }
    FLASH_Lock();
}


/**
  * 函    数：flash_write()
                f103c8t6    闪存共64页,闪存页大小为1KB (1024), 擦除以页大小为单位擦除
  * 参    数：start_addr    起始地址    
  * 参    数：page_num       写入内容的字节数
  * 返 回 值：无
  */
void flash_write(uint32_t start_addr, uint32_t *data, uint32_t len)
{
    FLASH_Unlock();
    while(len)
    {
        FLASH_ProgramWord(start_addr, *data);   //一次性写入4字节(uint32_t)
        len -= 4;
        start_addr += 4;
        data++;
    }
    FLASH_Lock();
}

