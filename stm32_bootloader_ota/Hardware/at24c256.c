#include "usart.h"
#include "at24c256.h"
#include "delay.h"  // 需自行实现延时函数

/* I2C初始化 */
void at24c256_init(void)
{
    /*开启时钟*/
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);		//开启I2C1的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟
    
    /*GPIO初始化*/
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);					//将PB10和PB11引脚初始化为复用开漏输出
    
    /*I2C初始化*/
    I2C_InitTypeDef I2C_InitStructure;						//定义结构体变量
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;				//模式，选择为I2C模式
    I2C_InitStructure.I2C_ClockSpeed = 100000;				//时钟速度，选择为100KHz
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;		//时钟占空比，选择Tlow/Thigh = 2
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;				//应答，选择使能
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;	//应答地址，选择7位，从机模式下才有效
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;				//自身地址，从机模式下才有效
    I2C_Init(I2C1, &I2C_InitStructure);						//将结构体变量交给I2C_Init，配置I2C1
    
    /*I2C使能*/
    I2C_Cmd(I2C1, ENABLE);									//使能I2C1，开始运行
}

/* 等待I2C事件 */
static ErrorStatus I2C_WaitEvent(uint32_t event)
{
    uint32_t timeout = 10000;
    while (!I2C_CheckEvent(I2C1, event)) {
        if (timeout-- == 0) {
            return ERROR;
        }
    }
    return SUCCESS;
}

/* 写一个字节到AT24C256 */
void at24c256_write_byte(uint16_t addr, uint8_t data)
{
    /* 发送起始信号 */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS) return;

    /* 发送设备地址（写模式） */
    I2C_Send7bitAddress(I2C1, AT24C256_ADDRESS, I2C_Direction_Transmitter);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != SUCCESS) return;

    /* 发送存储地址（高字节） */
    I2C_SendData(I2C1, (uint8_t)(addr >> 8));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;
    
    /* 发送存储地址（低字节） */
    I2C_SendData(I2C1, (uint8_t)(addr & 0xFF));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;

    /* 发送数据 */
    I2C_SendData(I2C1, data);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;

    /* 发送停止信号 */
    I2C_GenerateSTOP(I2C1, ENABLE);
    
    /* 等待AT24C256内部写操作完成（最大10ms） */
    delay_ms(10);
}

/* 从AT24C256读取一个字节 */
uint8_t at24c256_read_byte(uint16_t addr)
{
    uint8_t data = 0;

    /* 发送起始信号 */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS) return 0;

    /* 发送设备地址（写模式） */
    I2C_Send7bitAddress(I2C1, AT24C256_ADDRESS, I2C_Direction_Transmitter);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != SUCCESS) return 0;

    /* 发送存储地址（高字节） */
    I2C_SendData(I2C1, (uint8_t)(addr >> 8));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return 0;
    
    /* 发送存储地址（低字节） */
    I2C_SendData(I2C1, (uint8_t)(addr & 0xFF));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return 0;

    /* 重新发送起始信号 */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS) return 0;

    /* 发送设备地址（读模式） */
    I2C_Send7bitAddress(I2C1, AT24C256_ADDRESS | 0x01, I2C_Direction_Receiver);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) != SUCCESS) return 0;

    /* 禁止应答并发送停止信号 */
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    I2C_GenerateSTOP(I2C1, ENABLE);

    /* 读取数据 */
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_RECEIVED) != SUCCESS) return 0;
    data = I2C_ReceiveData(I2C1);

    /* 重新使能应答 */
    I2C_AcknowledgeConfig(I2C1, ENABLE);

    return data;
}

/* 页写入（最多写入AT24C256_PAGE_SIZE字节，且不跨页） */
void at24c256_write_page(uint16_t addr, uint8_t *data, uint8_t len)
{
    uint8_t i;
    uint8_t write_len = len;
    
    /* 检查是否跨页（AT24C256每页64字节） */
    if ((addr & 0x003F) + len > AT24C256_PAGE_SIZE) {
        write_len = AT24C256_PAGE_SIZE - (addr & 0x003F);  // 截断到页边界
    }

    /* 发送起始信号 */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS) return;

    /* 发送设备地址（写模式） */
    I2C_Send7bitAddress(I2C1, AT24C256_ADDRESS, I2C_Direction_Transmitter);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != SUCCESS) return;

    /* 发送存储地址（高字节） */
    I2C_SendData(I2C1, (uint8_t)(addr >> 8));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;
    
    /* 发送存储地址（低字节） */
    I2C_SendData(I2C1, (uint8_t)(addr & 0xFF));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;

    /* 发送数据 */
    for (i = 0; i < write_len; i++) {
        I2C_SendData(I2C1, data[i]);
        if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;
    }

    /* 发送停止信号 */
    I2C_GenerateSTOP(I2C1, ENABLE);
    
    /* 等待写操作完成 */
    delay_ms(10);
}

/* 连续读取多个字节 */
void at24c256_read_buffer(uint16_t addr, uint8_t *buffer, uint16_t len)
{
    uint16_t i;

    /* 发送起始信号 */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS) return;

    /* 发送设备地址（写模式） */
    I2C_Send7bitAddress(I2C1, AT24C256_ADDRESS, I2C_Direction_Transmitter);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != SUCCESS) return;

    /* 发送存储地址（高字节） */
    I2C_SendData(I2C1, (uint8_t)(addr >> 8));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;
    
    /* 发送存储地址（低字节） */
    I2C_SendData(I2C1, (uint8_t)(addr & 0xFF));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;

    /* 重新发送起始信号 */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS) return;

    /* 发送设备地址（读模式） */
    I2C_Send7bitAddress(I2C1, AT24C256_ADDRESS | 0x01, I2C_Direction_Receiver);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) != SUCCESS) return;

    /* 读取数据 */
    for (i = 0; i < len; i++) {
        /* 最后一个字节禁止应答 */
        if (i == len - 1) {
            I2C_AcknowledgeConfig(I2C1, DISABLE);
            I2C_GenerateSTOP(I2C1, ENABLE);
        }

        if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_RECEIVED) != SUCCESS) return;
        buffer[i] = I2C_ReceiveData(I2C1);
    }

    /* 重新使能应答 */
    I2C_AcknowledgeConfig(I2C1, ENABLE);
}

///* 测试函数 */
//void AT24C256_Test(void)
//{
//    uint8_t test_data[64] = {0};
//    uint8_t read_data[64] = {0};
//    uint16_t i;
//    
//    /* 准备测试数据 */
//    for (i = 0; i < 64; i++) {
//        test_data[i] = i+2;
//    }
//    
//    /* 页写入测试（地址0x1000） */
//    AT24C256_WritePage(0x1000, test_data, 64);
//    
//    /* 读取验证 */
//    AT24C256_ReadBuffer(0x1000, read_data, 64);
//    
//    /* 验证结果 */
//    for (i = 0; i < 64; i++) {
//        if (read_data[i] != test_data[i]) {
//			printf("\r\n AT24C256 init error \r\n");
//			break;
//        }
//		else
//		{
//			printf("\r\n AT24C256 init success \r\n");
//			for (i = 0; i < 64; i++) 
//			{
//				printf("\r\n test_data %d : %x\r\n",i,test_data[i]);
//			}
//			
//		}
//    }
//	printf("\r\n AT24C256 init ok \r\n");
//}

