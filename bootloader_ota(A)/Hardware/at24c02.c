#include "i2c.h"
#include "at24c02.h"
#include "delay.h"

//初始化IIC接口
void at24cxx_init(void)
{
    i2c_init();
    IIC_SDA = 1;
    IIC_SCL = 1;
}
//在AT24CXX指定地址读出一个数据
//ReadAddr:开始读数的地址
//返回值  :读到的数据
uint8_t at24cxx_read_byte(uint16_t ReadAddr)
{
    uint8_t temp = 0;
    i2c_start();
    if (EE_TYPE > AT24C16)
    {
        i2c_send_byte(0XA0);       //发送写命令
        i2c_wait_ack();
        i2c_send_byte(ReadAddr >> 8); //发送高地址
    }
    else i2c_send_byte(0XA0 + ((ReadAddr / 256) << 1)); //发送器件地址0XA0,写数据

    i2c_wait_ack();
    i2c_send_byte(ReadAddr % 256); //发送低地址
    i2c_wait_ack();
    i2c_start();
    i2c_send_byte(0XA1);           //进入接收模式
    i2c_wait_ack();
    temp = i2c_read_byte(0);
    i2c_stop();//产生一个停止条件
    return temp;
}
//在AT24CXX指定地址写入一个数据
//WriteAddr  :写入数据的目的地址
//DataToWrite:要写入的数据
void at24cxx_write_byte(uint16_t WriteAddr, uint8_t DataToWrite)
{
    i2c_start();
    if (EE_TYPE > AT24C16)
    {
        i2c_send_byte(0XA0);        //发送写命令
        i2c_wait_ack();
        i2c_send_byte(WriteAddr >> 8); //发送高地址
    }
    else
    {
        i2c_send_byte(0XA0 + ((WriteAddr / 256) << 1)); //发送器件地址0XA0,写数据
    }
    i2c_wait_ack();
    i2c_send_byte(WriteAddr % 256); //发送低地址
    i2c_wait_ack();
    i2c_send_byte(DataToWrite);     //发送字节
    i2c_wait_ack();
    i2c_stop();//产生一个停止条件
    delay_ms(10);
}
//在AT24CXX里面的指定地址开始写入长度为Len的数据
//该函数用于写入16bit或者32bit的数据.
//WriteAddr  :开始写入的地址
//DataToWrite:数据数组首地址
//Len        :要写入数据的长度2,4
void at24cxx_write_len_byte(uint16_t WriteAddr, uint32_t DataToWrite, uint8_t Len)
{
    uint8_t t;
    for (t = 0; t < Len; t++)
    {
        at24cxx_write_byte(WriteAddr + t, (DataToWrite >> (8 * t)) & 0xff);
    }
}

//在AT24CXX里面的指定地址开始读出长度为Len的数据
//该函数用于读出16bit或者32bit的数据.
//ReadAddr   :开始读出的地址
//返回值     :数据
//Len        :要读出数据的长度2,4
uint32_t at24cxx_read_len_byte(uint16_t ReadAddr, uint8_t Len)
{
    uint8_t t;
    uint32_t temp = 0;
    for (t = 0; t < Len; t++)
    {
        temp <<= 8;
        temp += at24cxx_read_byte(ReadAddr + Len - t - 1);
    }
    return temp;
}
//检查AT24CXX是否正常
//这里用了24XX的最后一个地址(255)来存储标志字.
//如果用其他24C系列,这个地址要修改
//返回1:检测失败
//返回0:检测成功
uint8_t at24cxx_check(void)
{
    uint8_t temp;
    temp = at24cxx_read_byte(255); //避免每次开机都写AT24CXX
    if (temp == 0x36)return 0;
    else//排除第一次初始化的情况
    {
        at24cxx_write_byte(255, 0x36);
        temp = at24cxx_read_byte(255);
        if (temp == 0x36)return 0;
    }
    return 1;
}

//在AT24CXX里面的指定地址开始读出指定个数的数据
//ReadAddr :开始读出的地址 对24c02为0~255
//pBuffer  :数据数组首地址
//NumToRead:要读出数据的个数
void at24cxx_read(uint16_t ReadAddr, uint8_t *pBuffer, uint16_t NumToRead)
{
    while (NumToRead)
    {
        *pBuffer++ = at24cxx_read_byte(ReadAddr++);
        NumToRead--;
    }
}
//在AT24CXX里面的指定地址开始写入指定个数的数据
//WriteAddr :开始写入的地址 对24c02为0~255
//pBuffer   :数据数组首地址
//NumToWrite:要写入数据的个数
void at24cxx_write(uint16_t WriteAddr, uint8_t *pBuffer, uint16_t NumToWrite)
{
    while (NumToWrite--)
    {
        at24cxx_write_byte(WriteAddr, *pBuffer);
        WriteAddr++;
        pBuffer++;
    }
}
