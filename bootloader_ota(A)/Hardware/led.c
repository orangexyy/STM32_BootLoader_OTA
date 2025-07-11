#include "led.h"     

void led_init(void)
{
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOC,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOC,GPIO_Pin_13);
}

void led1_on(void)		
{
	GPIO_ResetBits(GPIOC ,GPIO_Pin_13);
}

void led1_off(void)
{
	GPIO_SetBits(GPIOC,GPIO_Pin_13);
}

void led1_toggle(void)
{
	if(GPIO_ReadOutputDataBit(GPIOC,GPIO_Pin_13) == 0)
	{
		GPIO_SetBits(GPIOC,GPIO_Pin_13);
	}
	else
	{
		GPIO_ResetBits(GPIOC,GPIO_Pin_13);
	}
}


