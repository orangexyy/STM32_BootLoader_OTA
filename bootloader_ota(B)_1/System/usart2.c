#include "usart2.h"
#include "usart3.h"
#include "stdbool.h"
#include "stdarg.h"
#include "string.h"
#include "stdio.h"

volatile bool usart2_receive_flag = false;    //串口是否已接收完毕
uint16_t usart2_rx_len;
uint8_t usart2_rx_buffer[USART2_RX_SIZE];
uint8_t usart2_tx_buffer[USART2_TX_SIZE];

void USART2_IRQHandler(void)
{
    if( USART_GetITStatus(USART2, USART_IT_IDLE) != RESET )
    {
        USART2 -> SR;  //访问一下SR寄存器
        USART2 -> DR;  //访问一下DR寄存器

        DMA_Cmd(DMA1_Channel6, DISABLE);
        usart2_rx_len = USART2_RX_MAX - DMA_GetCurrDataCounter(DMA1_Channel6);
        DMA_SetCurrDataCounter( DMA1_Channel6, USART2_RX_MAX);
        DMA_Cmd(DMA1_Channel6, ENABLE);

        usart2_rx_buffer[usart2_rx_len] = '\0';          //给最后一位补上结束符
        // usart2_printf("%s\r\n",usart2_rx_buffer);
			


        usart2_receive_flag = true;                             //标记一帧数据已接收完成
        USART_ClearITPendingBit(USART2, USART_IT_IDLE);         //清除IDLE中断标志位
    }
}


/* @brief 获取接收完毕的标志位，查看是否已接收完成。
 * @retval 1: 接收完成
 *         0: 未接收完成 */
uint8_t get_usart2_receive_flag(void)
{
    if(usart2_receive_flag == true)
    {
        usart2_receive_flag = false;
        return 1;//如果接收已完成，返回1
    }
    else
        return 0;//接收未完成，返回0
}

/* @brief 获取接收到的数据包。
 * @retval  接收到的数据包，是一个长度为256的uint8_t数组指针，有结束符'\0'。 */
void get_usart2_rx_buffer(char* buf)
{ 
	uint16_t i = 0;
	for(i=0; i<get_usart2_rx_len(); i++)
	{
		buf[i] = usart2_rx_buffer[i];
	}
}

/* @brief 获取接收到的数据包长度。
 * @retval 接收完一帧数据的长度 */
uint16_t get_usart2_rx_len(void)
{ return usart2_rx_len; }

/* @brief 清空数据包，实际是把第一位用结束符替代。*/
void clear_usart2_rx_buffer(void)
{ usart2_rx_buffer[0] = '\0'; }



void usart2_send_byte(uint8_t Byte)		//发送单个字符
{
	while(USART_GetFlagStatus(USART2,USART_FLAG_TXE) == RESET);
    USART_SendData(USART2,Byte);
}

void usart2_send_string(char *String)		//发送一串字符
{
	uint8_t i;
	for(i=0; String[i] != '\0'; i++)
	{
		usart2_send_byte(String[i]);
	}
}

uint32_t usart2_pow(uint32_t X, uint32_t Y)		//次方函数  Result = X的Y次方
{
	uint32_t Result = 1;
	while(Y--)
	{
		Result *= X;
	}
	return Result;
}

void usart2_send_number(uint32_t Number, uint8_t Length)		//发送数字
{
	uint16_t i;
	for(i = 0; i < Length ;i++)
	{
		usart2_send_byte(Number / usart2_pow(10, Length - i -1) % 10 + '0');
	}
}

// int fputc(int ch, FILE *f)
// { 	
// 	while((USART2->SR&0X40)==0);//循环发送,直到发送完毕   
// 	USART2->DR = (unsigned char) ch;      
// 	return ch;
// }

// void usart2_printf(char *format,...)		//打印可变参数 可移植
// {
// 	char String[100];
// 	va_list arg;
// 	va_start(arg,format);
// 	vsprintf(String,format,arg);
// 	va_end(arg);
// 	usart2_send_string(String);
// }

static void usart2_periph_init(unsigned int bound)
{
    /*初始化发送接收引脚，PA2(USART2_TX)，PA3(USART2_RX)。*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /*初始化USART2外设。*/
    USART_DeInit(USART2);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART2, &USART_InitStructure);
    USART_Cmd(USART2, ENABLE);
}

static void usart2_idle_interrupt_init(void)
{
    USART_ClearFlag(USART2, USART_IT_IDLE);
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);
    NVIC_InitTypeDef NVIC_InitStructure;
    /*已在主函数中设置优先级分组。*/
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

static void usart2_rx_dma_init(void)
{
    /*开启USART2的接收寄存器的DMA通道，查参考手册，是DMA1通道3。*/
    DMA_DeInit(DMA1_Channel6);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    DMA_InitTypeDef DMA_InitStructure = {
        .DMA_PeripheralBaseAddr = (uint32_t)&USART2->DR,
        .DMA_MemoryBaseAddr = (uint32_t)usart2_rx_buffer, 
        .DMA_DIR = DMA_DIR_PeripheralSRC,
        .DMA_BufferSize = sizeof(usart2_rx_buffer), 
        .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
        .DMA_MemoryInc = DMA_MemoryInc_Enable,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
        .DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
        .DMA_Mode = DMA_Mode_Normal,
        .DMA_Priority = DMA_Priority_High,
        .DMA_M2M = DMA_M2M_Disable,
    };
    DMA_Init(DMA1_Channel6, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel6, ENABLE);
    USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);
}

/* @brief 总初始化*/
void usart2_init(unsigned int bound)
{
    usart2_periph_init(bound);                  //初始化串口外设
    usart2_rx_dma_init();           //初始化接收寄存器的DMA通道
    usart2_idle_interrupt_init();   //初始化串口的空闲中断 
}

