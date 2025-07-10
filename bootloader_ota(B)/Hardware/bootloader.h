#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "stm32f10x.h"
#include "sys.h"       

typedef enum
{
    BOOTLOADER_EVENT_NONE,     //初始状态
    BOOTLOADER_EVENT_OTA,      //OTA更新A区
    BOOTLOADER_EVENT_ERASE_A,
    BOOTLOADER_EVENT_IAP_START,
    BOOTLOADER_EVENT_IAP_RECEIVE_DATA,
    BOOTLOADER_EVENT_IAP_END,
    BOOTLOADER_EVENT_SET_OTA_VERSION,
    BOOTLOADER_EVENT_GET_OTA_VERSION,
    BOOTLOADER_EVENT_DOWNLOAD_TO_EXTERNAL_FLASH,
    BOOTLOADER_EVENT_DOWNLOAD_FROM_EXTERNAL_FLASH,
    BOOTLOADER_EVENT_GET_SIZE_OF_EXTERNAL_FLASH,
    BOOTLOADER_EVENT_SYSTEM_RESET,

} BOOTLOADER_EVENT_DATA;

typedef struct 
{
    uint32_t time;   
    uint32_t receive_buf_num; 
    uint16_t receive_crc;   
    uint8_t  direction_flag;     //0；下载到a区 1：下载到外部flash
} XMODEM_PROTOCOL_DATA;

typedef enum
{
    XMODEM_PROTOCOL_STATE_START,     
    XMODEM_PROTOCOL_STATE_SEND_C,        
    XMODEM_PROTOCOL_STATE_RECEIVE_DATA,
    XMODEM_PROTOCOL_STATE_SEND_END,
    XMODEM_PROTOCOL_STATE_END,  
} XMODEM_PROTOCOL_STATE_DATA;

typedef void (*set_pc)(void);

uint8_t bootloader_enter_detect(uint8_t timeout);
void bootloader_enter_info_printf(void);
void bootloader_deinit_periph(void);
void bootloader_branch(void);

uint16_t xmodem_crc16(uint8_t *data, uint16_t datalen);

__asm void SET_SP(uint32_t addr);
void bootloader_load_a_block(uint32_t addr);
void bootloader_ota_a_block(void);
void bootloader_erase_a_block(void);
void bootloader_iap_start(void);
void bootloader_iap_receive(void);
void bootloader_iap_end(void);
uint8_t bootloader_set_ota_version(void);
void bootloader_get_ota_version(void);
uint8_t bootloader_select_flash_block(void);
void bootloader_get_size_of_external_flash(void);
void bootloader_system_reset(void);

void bootloader_event_detect(void);
void bootloader_event_handle(void);




#endif



