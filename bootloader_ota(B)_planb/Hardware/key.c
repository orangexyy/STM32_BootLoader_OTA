#include "delay.h"
#include "key.h"

void key_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
}

uint8_t key_scan(void)
{
	uint8_t key_num = 0;
	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)
	{
		key_num = 1;
	}
	else if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15) == 1)
	{
		key_num = 2;
	}
	return key_num;
}


