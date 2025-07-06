#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "stm32f10x.h"
#include "sys.h"       

typedef enum
{
    BOOTLOADER_EVENT_NONE,                          //初始状态
    BOOTLOADER_EVENT_OTA,                           //OTA更新A区
    BOOTLOADER_EVENT_ERASE_A,                       //擦除A区
    BOOTLOADER_EVENT_IAP,                           //串口IAP下载程序
    BOOTLOADER_EVENT_SET_OTA_VERSION,               //设置OTA版本号
    BOOTLOADER_EVENT_GET_OTA_VERSION,               //获取OTA版本号
    BOOTLOADER_EVENT_DOWNLOAD_TO_EXTERNAL_FLASH,    //向外部FLASH下载程序
    BOOTLOADER_EVENT_DOWNLOAD_FROM_EXTERNAL_FLASH,  //使用外部FLASH中的程序
    BOOTLOADER_EVENT_SYSTEM_RESET,                  //系统复位

} BOOTLOADER_EVENT_DATA;

typedef struct 
{   
    uint32_t time;                                  //发送C的延时时间
    uint8_t receive_flag;                           //接收到起始帧的标志
    uint32_t receive_buf_num;                       //接收到的数据包的包数，（一包128字节）
    uint16_t receive_crc;                           //接收数据包的CRC校验码

} XMODEM_PROTOCOL_DATA;

typedef enum
{
    XMODEM_PROTOCOL_STATE_NONE,                     //初始状态
    XMODEM_PROTOCOL_STATE_START,                    //开始
    XMODEM_PROTOCOL_STATE_SEND_C,                   //发送C
    XMODEM_PROTOCOL_STATE_RECEIVE_DATA,             //接收数据                 
    XMODEM_PROTOCOL_STATE_END,                      //结束
} XMODEM_PROTOCOL_STATE_DATA;

typedef void (*set_pc)(void);

void bootloader_branch(void);
void bootloader_deinit_periph(void);
uint8_t bootloader_enter_detect(uint8_t timeout);
void bootloader_enter_info_printf(void);
void bootloader_event_detect(void);
void bootloader_event_handle(void);
__asm void SET_SP(uint32_t addr);
void bootloader_load_a_block(uint32_t addr);
void bootloader_ota_a_block(void);
void bootloader_get_ota_version(void);
uint16_t xmodem_crc16(uint8_t *data, uint16_t datalen);
uint8_t xmodem_protocol_handle(void);


#endif



