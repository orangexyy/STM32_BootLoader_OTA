#ifndef __MAIN_H
#define __MAIN_H

#include "sys.h"
#include "stm32f10x.h"                  // Device header

#define FLASH_START_ADDR            0x08000000                                                  //FLASH起始地址
#define FLASH_PAGE_SIZE             1024                                                        //FLASH扇区大小
#define FLASH_PAGE_NUM              64                                                          //FLASH扇区总数
#define FLASH_BLOCK_B_PAGE_NUM      20                                                          //B区扇区个数
#define FLASH_BLOCK_A_PAGE_NUM      FLASH_PAGE_NUM - FLASH_BLOCK_B_PAGE_NUM                     //A区扇区个数
#define FLASH_BLOCK_A_START_PAGE    FLASH_BLOCK_B_PAGE_NUM                                      //A区起始扇区编号
#define FLASH_BLOCK_A_START_ADDR    FLASH_START_ADDR + FLASH_BLOCK_B_PAGE_NUM * FLASH_PAGE_SIZE //A区起始地址

#define OTA_SET_FLAG                0x11223347
#define OTA_INFO_DATA_SIZE          sizeof(OTA_INFO_DATA)
 
typedef struct 
{
    uint32_t    flag;                      //ota更新标志
    uint32_t    app_data_size[11];         //应用程序所占的内存大小，第0个代表OTA程序，后面的为存放在外部flash的应用程序
    uint8_t     version[16];                //VER-1.0.0  (9个字节)    vER-1.0.0 
} OTA_INFO_DATA;

typedef struct 
{
    uint8_t buffer[FLASH_PAGE_SIZE];
    uint32_t data_block_num;            //应用程序存放在外部flash的扇区位置，0为OTA程序放在0号block，后面的为存放在外部flash的应用程序
    
} UPDATE_A_DATA;


extern OTA_INFO_DATA ota_info_struct;
extern UPDATE_A_DATA update_a_struct;


#endif /* MAIN_H */

