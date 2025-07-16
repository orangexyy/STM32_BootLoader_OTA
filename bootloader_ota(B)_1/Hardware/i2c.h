#ifndef __I2C_H
#define __I2C_H

#include "sys.h"
#include "stm32f10x.h"                  // Device header

//IO方向设置
#define SDA_IN()  {GPIOB->CRH&=0XFFFF0FFF;GPIOB->CRH|=8<<12;}
#define SDA_OUT() {GPIOB->CRH&=0XFFFF0FFF;GPIOB->CRH|=3<<12;}

//IO操作函数	 
#define IIC_SCL    PBout(10) //SCL
#define IIC_SDA    PBout(11) //SDA	 
#define READ_SDA   PBin(11)  //输入SDA 

//IIC所有操作函数
void i2c_init(void);                        //初始化i2c的IO口				 
void i2c_start(void);				        //发送i2c开始信号
void i2c_stop(void);	  			        //发送i2c停止信号
void i2c_send_byte(uint8_t txd);			//i2c发送一个字节
uint8_t i2c_read_byte(unsigned char ack);   //i2c读取一个字节
uint8_t i2c_wait_ack(void); 				//i2c等待ACK信号
void i2c_ack(void);					        //i2c发送ACK信号
void i2c_nack(void);				        //i2c不发送ACK信号

 


#endif
