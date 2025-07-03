#include "flash.h"
#include "delay.h"

void flash_erase(uint16_t start_addr, uint16_t num)
{
    uint16_t i;
    FLASH_Unlock();
    for( i=0; i<num; i++)
    {
        FLASH_ErasePage((0x08000000 + start_addr * 1024) + (1024 * i));
    }
    FLASH_Lock();
}

void flash_write(uint32_t start_addr, uint32_t *data, uint32_t num)
{
    FLASH_Unlock();
    while(num)
    {
        FLASH_ProgramWord(start_addr, *data);
        num -= 4;
        start_addr += 4;
        data++;
    }
    FLASH_Lock();
}

