#include "sys.h"
#include "stm32f10x.h"
#include "stm32f10x_dma.h"
#include "usart.h"	
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_UCOS
#include "includes.h"					//ucos 使用	  
#endif

//uint8_t usart_rx_buffer[USART_RX_SIZE];
//usart_rx_buffer_ctr usart1_rx_ctr;

#if USE_USART2_3
unsigned int Usart2_RxPacket[100];				//"@ABC#*"
uint8_t Usart2_RxFlag;

unsigned int Usart3_RxPacket[100];				//"@ABC#*"
uint8_t Usart3_RxFlag;

#endif


//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 0
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{ 	
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
	USART1->DR = (unsigned char) ch;      
	return ch;
}
#endif

//void Usart3_rx_buffer_Init(void)
//{
//	usart1_rx_ctr.p_usart_rx_write 	= &usart1_rx_ctr.p_usart_rx_buffer[0];
//	usart1_rx_ctr.p_usart_rx_read 	= &usart1_rx_ctr.p_usart_rx_buffer[0];
//	usart1_rx_ctr.p_usart_rx_end 	= &usart1_rx_ctr.p_usart_rx_buffer[NUM-1];

//	usart1_rx_ctr.p_usart_rx_write->p_start = &usart_rx_buffer[0];
//	usart1_rx_ctr.usart_rx_count = 0;
//}

 
/************************************************************     串口一     *******/

void usart1_init(unsigned int bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
	
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    // 2 GPIO USART1_TX   GPIOA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; // PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure);          // 初始化GPIOA.9

    // USART1_RX	  GPIOA.10初始化
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;            // PA10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);                // 初始化GPIOA.10

    USART_DeInit(USART1);
    USART_InitStructure.USART_BaudRate = bound;                                     // 串口波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                     // 字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                          // 一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;                             // 无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                 // 收发模式
    USART_Init(USART1, &USART_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_Cmd(USART1, ENABLE);
}


// void Usart1_DMA_Init(void)
// {
//     DMA_InitTypeDef DMA_InitStructure;

//     /* 开启DMA时钟 */
//     RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

//     /* 配置DMA1通道5（接收） */
//     DMA_DeInit(DMA1_Channel5);
//     DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART1->DR);
//     DMA_InitStructure.DMA_MemoryBaseAddr = (u32)usart_rx_buffer;
//     DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
//     DMA_InitStructure.DMA_BufferSize = USART_RX_NUM;
//     DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//     DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//     DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//     DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//     DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
//     DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
//     DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
//     DMA_Init(DMA1_Channel5, &DMA_InitStructure);

//     // 启动DMA接收通道
//     DMA_Cmd(DMA1_Channel5, ENABLE);
// }

void USART1_IRQHandler(void)		//接受数据，进入中断函数发送文本数据包  
{
	if(USART_GetFlagStatus(USART1,USART_IT_IDLE) == SET)
	{
		// usart1_rx_ctr.usart_rx_count += (USART_RX_NUM + 1) - DMA_GetCurrDataCounter(DMA1_Channel5);
		// usart1_rx_ctr.p_usart_rx_write->p_end = &usart_rx_buffer[usart1_rx_ctr.usart_rx_count - 1];
		// usart1_rx_ctr.p_usart_rx_write++;
		// if(usart1_rx_ctr.p_usart_rx_write == usart1_rx_ctr.p_usart_rx_end)
		// {
		// 	usart1_rx_ctr.p_usart_rx_write = &usart1_rx_ctr.p_usart_rx_buffer[0];
		// }
		// if(USART_RX_SIZE - usart1_rx_ctr.usart_rx_count >= USART_RX_NUM)
		// {
		// 	usart1_rx_ctr.p_usart_rx_write->p_start = &usart_rx_buffer[usart1_rx_ctr.usart_rx_count];
		// }
		// else
		// {
		// 	usart1_rx_ctr.p_usart_rx_write->p_start = &usart_rx_buffer[0];
		// 	usart1_rx_ctr.usart_rx_count = 0;
		// }
		// DMA_Cmd(DMA1_Channel5, DISABLE);
		// DMA_SetCurrDataCounter(DMA1_Channel5, USART_RX_NUM + 1);
		
		// DMA_Cmd(DMA1_Channel5, ENABLE);
		// clear = USART1->SR;
		// clear = USART1->DR;
		// USART_ReceiveData(USART1);
		
	}
}



void usart1_send_byte(uint8_t Byte)		//发送单个字符
{
	USART_SendData(USART1,Byte);
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET);
}


void usart1_SendArray(uint8_t *Array, uint16_t Length)		//发送一个数组
{
	uint16_t i;
	for(i=0;i<Length;i++)
	{
		usart1_send_byte(Array[i]);
	}
}
void usart1_send_string(char *String)		//发送一串字符
{
	uint8_t i;
	for(i=0; String[i] != '\0'; i++)
	{
		usart1_send_byte(String[i]);
	}
}

uint32_t usart1_pow(uint32_t X, uint32_t Y)		//次方函数  Result = X的Y次方
{
	uint32_t Result = 1;
	while(Y--)
	{
		Result *= X;
	}
	return Result;
}

void usart1_send_number(uint32_t Number, uint8_t Length)		//发送数字
{
	uint16_t i;
	for(i = 0; i < Length ;i++)
	{
		usart1_send_byte(Number / usart1_pow(10, Length - i -1) % 10 + '0');
	}
}




#if USE_USART2_3
/************************************************************     串口二     *******/

void Usart2_Init(unsigned int bound)
{
   //GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE); //使能GPIOA时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);//使能USART2时钟
 

	//USART2端口配置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //GPIOA2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化PA2，PA3
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; //GPIOA3
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化PA2，PA3

   //USART2 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(USART2, &USART_InitStructure); //初始化串口2
	
	USART_Cmd(USART2, ENABLE);  //使能串口2
	
	//USART_ClearFlag(USART2, USART_FLAG_TC);
	
	USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启相关中断

	//Usart2 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;//串口2中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
}

void USART2_SendByte(uint8_t Byte)		//发送单个字符
{
	USART_SendData(USART2,Byte);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TXE) == RESET);
}


void USART2_SendArray(uint8_t *Array, uint16_t Length)		//发送一个数组
{
	uint16_t i;
	for(i=0;i<Length;i++)
	{
		USART2_SendByte(Array[i]);
	}
}
void USART2_SendString(char *String)		//发送一串字符
{
	uint8_t i;
	for(i=0; String[i] != '\0'; i++)
	{
		USART2_SendByte(String[i]);
	}
}

uint32_t USART2_Pow(uint32_t X, uint32_t Y)		//次方函数  Result = X的Y次方
{
	uint32_t Result = 1;
	while(Y--)
	{
		Result *= X;
	}
	return Result;
}

void USART2_SendNumber(uint32_t Number, uint8_t Length)		//发送数字
{
	uint16_t i;
	for(i = 0; i < Length ;i++)
	{
		USART2_SendByte(Number / USART2_Pow(10, Length - i -1) % 10 + '0');
	}
}


void USART2_IRQHandler(void)		//接受数据，进入中断函数发送文本数据包  
{
	static uint8_t RxState = 0;
	static uint8_t pRxPacket = 0;
	
	if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
	{
		uint8_t RxData = USART_ReceiveData(USART2);
		
		if(RxState == 0)
		{
			if(RxData == 0xFF && Usart2_RxFlag == 0)
			{
				RxState = 1;
				pRxPacket = 0;
			}
		}
		else if(RxState == 1)
		{
			if(RxData == 0xFE)
			{
				RxState = 2;
			}
			else
			{
				Usart2_RxPacket[pRxPacket] = RxData;
				pRxPacket++;
			}
		}
		else if(RxState == 2)
		{
			if(RxData == 0xEE)
			{
				RxState = 0;
				Usart2_RxPacket[pRxPacket] = '\0';
				Usart2_RxFlag = 1;
			}
		}
		

		USART_ClearITPendingBit(USART2,USART_IT_RXNE);
	}
}
 

/************************************************************     串口三     *******/

void Usart3_Init(unsigned int bound)
{
   //GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE); //使能GPIOB时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);//使能USART3时钟
 
	
	//USART3端口配置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //GPIOB10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11; //GPIOB11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_Init(GPIOB,&GPIO_InitStructure); 

   //USART3 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(USART3, &USART_InitStructure); //初始化串口3
	
	USART_Cmd(USART3, ENABLE);  //使能串口3 
	
	USART_ClearFlag(USART3, USART_FLAG_TC);
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//开启相关中断

	//Usart3 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;//串口3中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
}

void USART3_SendByte(uint8_t Byte)		//发送单个字符
{
	USART_SendData(USART3,Byte);
	while(USART_GetFlagStatus(USART3,USART_FLAG_TXE) == RESET);
}


void USART3_SendArray(uint8_t *Array, uint16_t Length)		//发送一个数组
{
	uint16_t i;
	for(i=0;i<Length;i++)
	{
		USART3_SendByte(Array[i]);
	}
}
void USART3_SendString(char *String)		//发送一串字符
{
	uint8_t i;
	for(i=0; String[i] != '\0'; i++)
	{
		USART3_SendByte(String[i]);
	}
}

uint32_t USART3_Pow(uint32_t X, uint32_t Y)		//次方函数  Result = X的Y次方
{
	uint32_t Result = 1;
	while(Y--)
	{
		Result *= X;
	}
	return Result;
}

void USART3_SendNumber(uint32_t Number, uint8_t Length)		//发送数字
{
	uint16_t i;
	for(i = 0; i < Length ;i++)
	{
		USART3_SendByte(Number / USART3_Pow(10, Length - i -1) % 10 + '0');
	}
}


void USART3_IRQHandler(void)		//接受数据，进入中断函数发送文本数据包  
{
	static uint8_t RxState = 0;
	static uint8_t pRxPacket = 0;
	
	if(USART_GetITStatus(USART3,USART_IT_RXNE) == SET)
	{
		uint8_t RxData = USART_ReceiveData(USART3);
		
		if(RxState == 0)
		{
			if(RxData ==0x99 && Usart3_RxFlag == 0)
			{
				RxState = 1;
				pRxPacket = 0;
			}
		}
		else if(RxState == 1)
		{
			if(RxData == 0x88)
			{
				RxState = 2;
			}
			else
			{
				Usart3_RxPacket[pRxPacket] = RxData;
				pRxPacket++;
			}
		}
		else if(RxState == 2)
		{
			if(RxData == 0x77)
			{
				RxState = 0;
				Usart3_RxPacket[pRxPacket] = 99;
				Usart3_RxFlag = 1;
			}
		}
		USART_ClearITPendingBit(USART3,USART_IT_RXNE);
	}
}

#endif 
