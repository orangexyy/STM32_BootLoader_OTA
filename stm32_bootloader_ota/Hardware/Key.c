#include "stm32f10x.h"                  // Device header
#include "delay.h"

uint8_t Key_adjust_light = 0;	//�����ȵ��м�ֵ
uint8_t Key_adjust_color = 0;	//����ɫ���м�ֵ
uint8_t Key_adjust_clock = 0;	//��ʱ�ӵ��м�ֵ
uint8_t Key_mode = 0;			//0~4;����  5:��ʱ  6������
uint8_t adjust_flag = 0;		//0�������� 1����ʱ��  2������ɫ
uint8_t change_flag = 0;		//0����ʱ��ʱ 1����ʱ�ӷ� 2��������ʱ 3�������ӷ� 4���������� 5��ȷ������


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
	uint32_t Temp = 200000;	//��ʱ�õļ�ʱ����
	uint8_t Key_adjust_clock = 0;
	
	if(change_flag == 0) 		// 0����ʱ�ӵ�ʱ  ����1
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)			//��PB11����Ĵ�����״̬�����Ϊ0���������1����
		{
			delay_ms(20);											//��ʱ����
			while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)	//�ȴ��������֣��򳤰���������ֵ
			{
				Temp--;
				if(Temp == 0)
				{
					delay_ms(10);
					return 1;
				}
			}
			delay_ms(20);											//��ʱ����
			Key_adjust_clock = 1;											//�ü���Ϊ1
		}
	}
	
	else if(change_flag == 1) 	// 1����ʱ�ӵķ�  ����2
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)			//��PB11����Ĵ�����״̬�����Ϊ0���������1����
		{
			delay_ms(20);											//��ʱ����
			while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)	//�ȴ��������֣��򳤰���������ֵ
			{
				Temp--;
				if(Temp == 0)
				{
					delay_ms(10);
					return 2;
				}
			}
			delay_ms(20);											//��ʱ����
			Key_adjust_clock = 2;											//�ü���Ϊ1
		}
	}
	
	else if(change_flag == 2) 	// 2�������ӵ�ʱ  ����3
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)			//��PB11����Ĵ�����״̬�����Ϊ0���������1����
		{
			delay_ms(20);											//��ʱ����
			while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)	//�ȴ��������֣��򳤰���������ֵ
			{
				Temp--;
				if(Temp == 0)
				{
					delay_ms(10);
					return 3;
				}
			}
			delay_ms(20);											//��ʱ����
			Key_adjust_clock = 3;											//�ü���Ϊ1
		}
	}
	
	else if(change_flag == 3) 	//3�������ӵķ�  ����4
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)			//��PB11����Ĵ�����״̬�����Ϊ0���������1����
		{
			delay_ms(20);											//��ʱ����
			while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)	//�ȴ��������֣��򳤰���������ֵ
			{
				Temp--;
				if(Temp == 0)
				{
					delay_ms(5);
					return 4;
				}
			}
			delay_ms(20);											//��ʱ����
			Key_adjust_clock = 4;											//�ü���Ϊ1
		}
	}
	
	else if(change_flag == 4) 	//4�������ӵ���  ����5
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)			//��PB11����Ĵ�����״̬�����Ϊ0���������1����
		{
			delay_ms(20);											//��ʱ����
			while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)	//�ȴ��������֣��򳤰���������ֵ
			{
				Temp--;
				if(Temp == 0)
				{
					delay_ms(5);
					return 5;
				}
			}
			delay_ms(20);											//��ʱ����
			Key_adjust_clock = 5;											//�ü���Ϊ1
		}
	}
	
	else if(change_flag == 5) 	//5��ȷ������ֵ  ����6
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)			//��PB11����Ĵ�����״̬�����Ϊ0���������1����
		{
			delay_ms(20);											//��ʱ����
			while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 1)	//�ȴ��������֣��򳤰���������ֵ
			{
				Temp--;
				if(Temp == 0)
				{
					delay_ms(5);
					return 6;
				}
			}
			delay_ms(20);											//��ʱ����
			Key_adjust_clock = 6;											//�ü���Ϊ1
		}
	}
	return Key_adjust_clock;
	
}

void Key_Control(uint8_t mode_old, uint8_t light_old, uint8_t color_old,
				uint8_t* mode_new, uint8_t* light_new, uint8_t* adjust_clock, uint8_t* color_new)
{
	
		Key_mode = mode_old;
	
		switch(mode_old)		//���жϵ�ǰģʽ
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




