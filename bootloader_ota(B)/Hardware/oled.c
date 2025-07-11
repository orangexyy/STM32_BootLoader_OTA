#include "oled.h"
#include "oled_font.h"



/*引脚配置*/
#define OLED_W_SCL(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_8, (BitAction)(x))
#define OLED_W_SDA(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_9, (BitAction)(x))

/*引脚初始化*/
void oled_i2c_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C开始
  * @param  无
  * @retval 无
  */
void oled_i2c_start(void)
{
	OLED_W_SDA(1);
	OLED_W_SCL(1);
	OLED_W_SDA(0);
	OLED_W_SCL(0);
}

/**
  * @brief  I2C停止
  * @param  无
  * @retval 无
  */
void oled_i2c_stop(void)
{
	OLED_W_SDA(0);
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C发送一个字节
  * @param  Byte 要发送的一个字节
  * @retval 无
  */
void oled_i2c_send_byte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_W_SDA(Byte & (0x80 >> i));
		OLED_W_SCL(1);
		OLED_W_SCL(0);
	}
	OLED_W_SCL(1);	//额外的一个时钟，不处理应答信号
	OLED_W_SCL(0);
}

/**
  * @brief  OLED写命令
  * @param  Command 要写入的命令
  * @retval 无
  */
void oled_write_command(uint8_t Command)
{
	oled_i2c_start();
	oled_i2c_send_byte(0x78);		//从机地址
	oled_i2c_send_byte(0x00);		//写命令
	oled_i2c_send_byte(Command); 
	oled_i2c_stop();
}

/**
  * @brief  OLED写数据
  * @param  Data 要写入的数据
  * @retval 无
  */
void oled_write_data(uint8_t Data)
{
	oled_i2c_start();
	oled_i2c_send_byte(0x78);		//从机地址
	oled_i2c_send_byte(0x40);		//写数据
	oled_i2c_send_byte(Data);
	oled_i2c_stop();
}

/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval 无
  */
void oled_set_cursor(uint8_t Y, uint8_t X)
{
	oled_write_command(0xB0 | Y);					//设置Y位置
	oled_write_command(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
	oled_write_command(0x00 | (X & 0x0F));			//设置X位置低4位
}

/**
  * @brief  OLED清屏
  * @param  无
  * @retval 无
  */
void oled_clear(void)
{  
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		oled_set_cursor(j, 0);
		for(i = 0; i < 128; i++)
		{
			oled_write_data(0x00);
		}
	}
}

/**
  * @brief  OLED显示一个字符
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Char 要显示的一个字符，范围：ASCII可见字符
  * @retval 无
  */
void oled_show_char(uint8_t Line, uint8_t Column, char Char)
{      	
	uint8_t i;
	oled_set_cursor((Line - 1) * 2, (Column - 1) * 8);		//设置光标位置在上半部分
	for (i = 0; i < 8; i++)
	{
		oled_write_data(OLED_F8x16[Char - ' '][i]);			//显示上半部分内容
	}
	oled_set_cursor((Line - 1) * 2 + 1, (Column - 1) * 8);	//设置光标位置在下半部分
	for (i = 0; i < 8; i++)
	{
		oled_write_data(OLED_F8x16[Char - ' '][i + 8]);		//显示下半部分内容
	}
}

/**
  * @brief  OLED显示字符串
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  String 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
void oled_show_string(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		oled_show_char(Line, Column + i, String[i]);
	}
}

/**
  * @brief  OLED次方函数
  * @retval 返回值等于X的Y次方
  */
uint32_t oled_pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
  * @brief  OLED显示数字（十进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~4294967295
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void oled_show_num(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		oled_show_char(Line, Column + i, Number / oled_pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十进制，带符号数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：-2147483648~2147483647
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void oled_show_signednum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		oled_show_char(Line, Column, '+');
		Number1 = Number;
	}
	else
	{
		oled_show_char(Line, Column, '-');
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)							
	{
		oled_show_char(Line, Column + i + 1, Number1 / oled_pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十六进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
  * @param  Length 要显示数字的长度，范围：1~8
  * @retval 无
  */
void oled_show_hexnum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / oled_pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			oled_show_char(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			oled_show_char(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

/**
  * @brief  OLED显示数字（二进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
  * @param  Length 要显示数字的长度，范围：1~16
  * @retval 无
  */
void oled_show_binnum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		oled_show_char(Line, Column + i, Number / oled_pow(2, Length - i - 1) % 2 + '0');
	}
}

/**
  * @brief  OLED初始化
  * @param  无
  * @retval 无
  */
void oled_init(void)
{
	uint32_t i, j;
	
	for (i = 0; i < 1000; i++)			//上电延时
	{
		for (j = 0; j < 1000; j++);
	}
	
	oled_i2c_init();			//端口初始化
	
	oled_write_command(0xAE);	//关闭显示
	
	oled_write_command(0xD5);	//设置显示时钟分频比/振荡器频率
	oled_write_command(0x80);
	
	oled_write_command(0xA8);	//设置多路复用率
	oled_write_command(0x3F);
	
	oled_write_command(0xD3);	//设置显示偏移
	oled_write_command(0x00);
	
	oled_write_command(0x40);	//设置显示开始行
	
	oled_write_command(0xA1);	//设置左右方向，0xA1正常 0xA0左右反置
	
	oled_write_command(0xC8);	//设置上下方向，0xC8正常 0xC0上下反置

	oled_write_command(0xDA);	//设置COM引脚硬件配置
	oled_write_command(0x12);
	
	oled_write_command(0x81);	//设置对比度控制
	oled_write_command(0xCF);

	oled_write_command(0xD9);	//设置预充电周期
	oled_write_command(0xF1);

	oled_write_command(0xDB);	//设置VCOMH取消选择级别
	oled_write_command(0x30);

	oled_write_command(0xA4);	//设置整个显示打开/关闭

	oled_write_command(0xA6);	//设置正常/倒转显示

	oled_write_command(0x8D);	//设置充电泵
	oled_write_command(0x14);

	oled_write_command(0xAF);	//开启显示
		
	oled_clear();				//OLED清屏
}
