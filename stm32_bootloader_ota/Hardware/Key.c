#include "stm32f10x.h"                  // Device header
#include "delay.h"

uint8_t Key_adjust_light = 0;	//调亮度的中间值
uint8_t Key_adjust_color = 0;	//调光色的中间值
uint8_t Key_adjust_clock = 0;	//调时钟的中间值
uint8_t Key_mode = 0;			//0~4;亮度  5:调时  6：调分
uint8_t adjust_flag = 0;		//0：调亮度 1：调时钟  2：调颜色
uint8_t change_flag = 0;		//0：调时钟时 1：调时钟分 2：调闹钟时 3：调闹钟分 4：调闹钟秒 5：确认闹钟


void Key_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
}

uint8_t Key_Press(void)
{
	uint8_t KeyNum = 0;
	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)
	{
		KeyNum = 1;
	}
	else if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15) == 1)
	{
		KeyNum = 2;
	}

	return KeyNum;
}



uint8_t Key_Adjust_Light(uint8_t light_old)
{
	Key_adjust_light = light_old;
	
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_14) == 1)
	{
		delay_ms(20);
		while(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_14) == 1);
		delay_ms(20);
		Key_adjust_light++;
		if(Key_adjust_light == 5) Key_adjust_light = 0;
	}
	
	return Key_adjust_light;
}

uint8_t Key_Adjust_Color()
{
	
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_14) == 1)
	{
		delay_ms(20);
		while(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_14) == 1);
		delay_ms(20);
		Key_adjust_color++;
		if(Key_adjust_color == 7) Key_adjust_color = 0;
	}
	
	return Key_adjust_color;
}


uint8_t Key_Adjust_Clock(void)
{
	uint32_t Temp = 200000;	//临时用的计时变量
	uint8_t Key_adjust_clock = 0;
	
	if(change_flag == 0) 		// 0：调时钟的时  返回1
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)			//读PB11输入寄存器的状态，如果为0，则代表按键1按下
		{
			delay_ms(20);											//延时消抖
			while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)	//等待按键松手，或长按连续返回值
			{
				Temp--;
				if(Temp == 0)
				{
					delay_ms(10);
					return 1;
				}
			}
			delay_ms(20);											//延时消抖
			Key_adjust_clock = 1;											//置键码为1
		}
	}
	
	else if(change_flag == 1) 	// 1：调时钟的分  返回2
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)			//读PB11输入寄存器的状态，如果为0，则代表按键1按下
		{
			delay_ms(20);											//延时消抖
			while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)	//等待按键松手，或长按连续返回值
			{
				Temp--;
				if(Temp == 0)
				{
					delay_ms(10);
					return 2;
				}
			}
			delay_ms(20);											//延时消抖
			Key_adjust_clock = 2;											//置键码为1
		}
	}
	
	else if(change_flag == 2) 	// 2：调闹钟的时  返回3
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)			//读PB11输入寄存器的状态，如果为0，则代表按键1按下
		{
			delay_ms(20);											//延时消抖
			while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)	//等待按键松手，或长按连续返回值
			{
				Temp--;
				if(Temp == 0)
				{
					delay_ms(10);
					return 3;
				}
			}
			delay_ms(20);											//延时消抖
			Key_adjust_clock = 3;											//置键码为1
		}
	}
	
	else if(change_flag == 3) 	//3：调闹钟的分  返回4
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)			//读PB11输入寄存器的状态，如果为0，则代表按键1按下
		{
			delay_ms(20);											//延时消抖
			while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)	//等待按键松手，或长按连续返回值
			{
				Temp--;
				if(Temp == 0)
				{
					delay_ms(5);
					return 4;
				}
			}
			delay_ms(20);											//延时消抖
			Key_adjust_clock = 4;											//置键码为1
		}
	}
	
	else if(change_flag == 4) 	//4：调闹钟的秒  返回5
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)			//读PB11输入寄存器的状态，如果为0，则代表按键1按下
		{
			delay_ms(20);											//延时消抖
			while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)	//等待按键松手，或长按连续返回值
			{
				Temp--;
				if(Temp == 0)
				{
					delay_ms(5);
					return 5;
				}
			}
			delay_ms(20);											//延时消抖
			Key_adjust_clock = 5;											//置键码为1
		}
	}
	
	else if(change_flag == 5) 	//5：确定闹钟值  返回6
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)			//读PB11输入寄存器的状态，如果为0，则代表按键1按下
		{
			delay_ms(20);											//延时消抖
			while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)	//等待按键松手，或长按连续返回值
			{
				Temp--;
				if(Temp == 0)
				{
					delay_ms(5);
					return 6;
				}
			}
			delay_ms(20);											//延时消抖
			Key_adjust_clock = 6;											//置键码为1
		}
	}
	return Key_adjust_clock;
	
}

void Key_Control(uint8_t mode_old, uint8_t light_old, uint8_t color_old,
				uint8_t* mode_new, uint8_t* light_new, uint8_t* adjust_clock, uint8_t* color_new)
{
	
		Key_mode = mode_old;
	
		switch(mode_old)		//先判断当前模式
		{
			case 0:adjust_flag = 0;break;
			case 1:adjust_flag = 0;break;
			case 2:adjust_flag = 0;break;
			case 3:adjust_flag = 0;break;
			case 4:adjust_flag = 0;break;
			case 5:adjust_flag = 0;break;
			case 6:adjust_flag = 2;break;
			case 7:change_flag = 0;adjust_flag = 1;break;
			case 8:change_flag = 1;adjust_flag = 1;break;
			case 9:change_flag = 2;adjust_flag = 1;break;
			case 10:change_flag = 3;adjust_flag = 1;break;
			case 11:change_flag = 4;adjust_flag = 1;break;
			case 12:change_flag = 5;adjust_flag = 1;break;
			default:adjust_flag = 0;break;
		}
		
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_15) == 1)
	{
		delay_ms(20);
		while(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_15) == 1);
		delay_ms(20);
		
		Key_mode++;
		if(Key_mode == 13) Key_mode = 0;
		
		*mode_new = Key_mode;
	}	
	
	if(adjust_flag == 0)
	{
		*light_new = Key_Adjust_Light(light_old);
		if(Key_mode == 3 || Key_mode == 4 || Key_mode == 5) 
		{
			*light_new = 0;
			*color_new = 0;
		}
		
	}
	else if(adjust_flag == 1)
	{
		*adjust_clock = Key_Adjust_Clock();
		*color_new = 0;
	}
	else if(adjust_flag == 2)
	{
		*color_new = Key_Adjust_Color();
	}
	
		
	
	

}




