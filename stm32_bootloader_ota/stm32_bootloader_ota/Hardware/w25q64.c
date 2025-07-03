#include "spi.h"
#include "w25q64_ins.h"

/**
  * 函    数：W25Q64初始化
  * 参    数：无
  * 返 回 值：无
  */
void w25q64_init(void)
{
	spi_init();					//先初始化底层的SPI
}

/**
  * 函    数：W25Q64读取ID号
  * 参    数：MID 工厂ID，使用输出参数的形式返回
  * 参    数：DID 设备ID，使用输出参数的形式返回
  * 返 回 值：无
  */
void w25q64_read_id(uint8_t *mid, uint16_t *did)
{
	spi_start();								//SPI起始
	spi_write_read_byte(W25Q64_JEDEC_ID);			//交换发送读取ID的指令
	*mid = spi_write_read_byte(W25Q64_DUMMY_BYTE);	//交换接收MID，通过输出参数返回
	*did = spi_write_read_byte(W25Q64_DUMMY_BYTE);	//交换接收DID高8位
	*did <<= 8;									//高8位移到高位
	*did |= spi_write_read_byte(W25Q64_DUMMY_BYTE);	//或上交换接收DID的低8位，通过输出参数返回
	spi_stop();								//SPI终止
}

/**
  * 函    数：W25Q64写使能
  * 参    数：无
  * 返 回 值：无
  */
void w25q64_write_enable(void)
{
	spi_start();								//SPI起始
	spi_write_read_byte(W25Q64_WRITE_ENABLE);		//交换发送写使能的指令
	spi_stop();								//SPI终止
}

/**
  * 函    数：W25Q64等待忙
  * 参    数：无
  * 返 回 值：无
  */
void w25q64_wait_busy(void)
{
	uint32_t timeout;
	spi_start();								//SPI起始
	spi_write_read_byte(W25Q64_READ_STATUS_REGISTER_1);				//交换发送读状态寄存器1的指令
	timeout = 100000;							//给定超时计数时间
	while ((spi_write_read_byte(W25Q64_DUMMY_BYTE) & 0x01) == 0x01)	//循环等待忙标志位
	{
		timeout --;								//等待时，计数值自减
		if (timeout == 0)						//自减到0后，等待超时
		{
			/*超时的错误处理代码，可以添加到此处*/
			break;								//跳出等待，不等了
		}
	}
	spi_stop();								//SPI终止
}

/**
  * 函    数：W25Q64页编程
  * 参    数：Address 页编程的起始地址，范围：0x000000~0x7FFFFF
  * 参    数：DataArray	用于写入数据的数组
  * 参    数：Count 要写入数据的数量，范围：0~256
  * 返 回 值：无
  * 注意事项：写入的地址范围不能跨页
  */
// void W25Q64_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count)
// {
// 	uint16_t i;
	
// 	W25Q64_WriteEnable();						//写使能
	
// 	MySPI_Start();								//SPI起始
// 	MySPI_SwapByte(W25Q64_PAGE_PROGRAM);		//交换发送页编程的指令
// 	MySPI_SwapByte(Address >> 16);				//交换发送地址23~16位
// 	MySPI_SwapByte(Address >> 8);				//交换发送地址15~8位
// 	MySPI_SwapByte(Address);					//交换发送地址7~0位
// 	for (i = 0; i < Count; i ++)				//循环Count次
// 	{
// 		MySPI_SwapByte(DataArray[i]);			//依次在起始地址后写入数据
// 	}
// 	MySPI_Stop();								//SPI终止
	
// 	W25Q64_WaitBusy();							//等待忙
// }

void w25q64_write_page(uint32_t addr, uint8_t *buf, uint16_t len)
{
	uint16_t i;
	
	w25q64_write_enable();							//写使能
	
	spi_start();									//SPI起始
	spi_write_read_byte(W25Q64_PAGE_PROGRAM);		//交换发送页编程的指令
	spi_write_read_byte((addr*256) >> 16);			//交换发送地址23~16位
	spi_write_read_byte((addr*256) >> 8);			//交换发送地址15~8位
	spi_write_read_byte((addr*256) >> 0);			//交换发送地址7~0位
	for (i = 0; i < len; i ++)						//循环len次
	{
		spi_write_read_byte(buf[i]);				//依次在起始地址后写入数据
	}
	spi_stop();										//SPI终止
	
	w25q64_wait_busy();								//等待忙
}

/**
  * 函    数：W25Q64扇区擦除（4KB）
  * 参    数：Address 指定扇区的地址，范围：0x000000~0x7FFFFF
  * 返 回 值：无
  */
// void W25Q64_SectorErase(uint32_t Address)
// {
// 	W25Q64_WriteEnable();						//写使能
	
// 	MySPI_Start();								//SPI起始
// 	MySPI_SwapByte(W25Q64_SECTOR_ERASE_4KB);	//交换发送扇区擦除的指令
// 	MySPI_SwapByte(Address >> 16);				//交换发送地址23~16位
// 	MySPI_SwapByte(Address >> 8);				//交换发送地址15~8位
// 	MySPI_SwapByte(Address);					//交换发送地址7~0位
// 	MySPI_Stop();								//SPI终止
	
// 	W25Q64_WaitBusy();							//等待忙
// }

void w25q64_sector_erase_64k(uint32_t addr)
{
	w25q64_write_enable();							//写使能
	
	spi_start();									//SPI起始
	spi_write_read_byte(W25Q64_BLOCK_ERASE_64KB);	//交换发送扇区擦除的指令
	spi_write_read_byte((addr*64*1024) >> 16);		//交换发送地址23~16位
	spi_write_read_byte((addr*64*1024) >> 8);		//交换发送地址15~8位
	spi_write_read_byte((addr*64*1024) >> 0);		//交换发送地址7~0位
	spi_stop();										//SPI终止
	
	w25q64_wait_busy();								//等待忙
}

/**
  * 函    数：W25Q64读取数据
  * 参    数：Address 读取数据的起始地址，范围：0x000000~0x7FFFFF
  * 参    数：DataArray 用于接收读取数据的数组，通过输出参数返回
  * 参    数：Count 要读取数据的数量，范围：0~0x800000
  * 返 回 值：无
  */
void w25q64_read_data(uint32_t addr, uint8_t *buf, uint32_t len)
{
	uint32_t i;
	spi_start();											//SPI起始
	spi_write_read_byte(W25Q64_READ_DATA);					//交换发送读取数据的指令
	spi_write_read_byte(addr >> 16);						//交换发送地址23~16位
	spi_write_read_byte(addr >> 8);							//交换发送地址15~8位
	spi_write_read_byte(addr);								//交换发送地址7~0位
	for (i = 0; i < len; i ++)								//循环len次
	{
		buf[i] = spi_write_read_byte(W25Q64_DUMMY_BYTE);	//依次在起始地址后读取数据
	}
	spi_stop();												//SPI终止
}
