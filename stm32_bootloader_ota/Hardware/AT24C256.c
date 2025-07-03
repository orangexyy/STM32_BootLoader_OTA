#include "stm32f10x.h"                  // Device header
#include "usart.h"
#include "at24c256.h"
#include "delay.h"  // ������ʵ����ʱ����

/* I2C��ʼ�� */
void I2C_Configuration(void)
{
    /*����ʱ��*/
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);		//����I2C1��ʱ��
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//����GPIOB��ʱ��
		
		/*GPIO��ʼ��*/
		GPIO_InitTypeDef GPIO_InitStructure;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOB, &GPIO_InitStructure);					//��PB10��PB11���ų�ʼ��Ϊ���ÿ�©���
		
		/*I2C��ʼ��*/
		I2C_InitTypeDef I2C_InitStructure;						//����ṹ�����
		I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;				//ģʽ��ѡ��ΪI2Cģʽ
		I2C_InitStructure.I2C_ClockSpeed = 100000;				//ʱ���ٶȣ�ѡ��Ϊ100KHz
		I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;		//ʱ��ռ�ձȣ�ѡ��Tlow/Thigh = 2
		I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;				//Ӧ��ѡ��ʹ��
		I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;	//Ӧ���ַ��ѡ��7λ���ӻ�ģʽ�²���Ч
		I2C_InitStructure.I2C_OwnAddress1 = 0x00;				//�����ַ���ӻ�ģʽ�²���Ч
		I2C_Init(I2C1, &I2C_InitStructure);						//���ṹ���������I2C_Init������I2C1
		
		/*I2Cʹ��*/
		I2C_Cmd(I2C1, ENABLE);									//ʹ��I2C1����ʼ����
}

/* �ȴ�I2C�¼� */
static ErrorStatus I2C_WaitEvent(uint32_t event)
{
    uint32_t timeout = 10000;
    while (!I2C_CheckEvent(I2C1, event)) {
        if (timeout-- == 0) {
            return ERROR;
        }
    }
    return SUCCESS;
}

/* дһ���ֽڵ�AT24C256 */
void AT24C256_WriteByte(uint16_t addr, uint8_t data)
{
    /* ������ʼ�ź� */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS) return;

    /* �����豸��ַ��дģʽ�� */
    I2C_Send7bitAddress(I2C1, AT24C256_ADDRESS, I2C_Direction_Transmitter);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != SUCCESS) return;

    /* ���ʹ洢��ַ�����ֽڣ� */
    I2C_SendData(I2C1, (uint8_t)(addr >> 8));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;
    
    /* ���ʹ洢��ַ�����ֽڣ� */
    I2C_SendData(I2C1, (uint8_t)(addr & 0xFF));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;

    /* �������� */
    I2C_SendData(I2C1, data);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;

    /* ����ֹͣ�ź� */
    I2C_GenerateSTOP(I2C1, ENABLE);
    
    /* �ȴ�AT24C256�ڲ�д������ɣ����10ms�� */
    delay_ms(10);
}

/* ��AT24C256��ȡһ���ֽ� */
uint8_t AT24C256_ReadByte(uint16_t addr)
{
    uint8_t data = 0;

    /* ������ʼ�ź� */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS) return 0;

    /* �����豸��ַ��дģʽ�� */
    I2C_Send7bitAddress(I2C1, AT24C256_ADDRESS, I2C_Direction_Transmitter);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != SUCCESS) return 0;

    /* ���ʹ洢��ַ�����ֽڣ� */
    I2C_SendData(I2C1, (uint8_t)(addr >> 8));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return 0;
    
    /* ���ʹ洢��ַ�����ֽڣ� */
    I2C_SendData(I2C1, (uint8_t)(addr & 0xFF));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return 0;

    /* ���·�����ʼ�ź� */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS) return 0;

    /* �����豸��ַ����ģʽ�� */
    I2C_Send7bitAddress(I2C1, AT24C256_ADDRESS | 0x01, I2C_Direction_Receiver);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) != SUCCESS) return 0;

    /* ��ֹӦ�𲢷���ֹͣ�ź� */
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    I2C_GenerateSTOP(I2C1, ENABLE);

    /* ��ȡ���� */
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_RECEIVED) != SUCCESS) return 0;
    data = I2C_ReceiveData(I2C1);

    /* ����ʹ��Ӧ�� */
    I2C_AcknowledgeConfig(I2C1, ENABLE);

    return data;
}

/* ҳд�루���д��AT24C256_PAGE_SIZE�ֽڣ��Ҳ���ҳ�� */
void AT24C256_WritePage(uint16_t addr, uint8_t *data, uint8_t len)
{
    uint8_t i;
    uint8_t write_len = len;
    
    /* ����Ƿ��ҳ��AT24C256ÿҳ64�ֽڣ� */
    if ((addr & 0x003F) + len > AT24C256_PAGE_SIZE) {
        write_len = AT24C256_PAGE_SIZE - (addr & 0x003F);  // �ضϵ�ҳ�߽�
    }

    /* ������ʼ�ź� */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS) return;

    /* �����豸��ַ��дģʽ�� */
    I2C_Send7bitAddress(I2C1, AT24C256_ADDRESS, I2C_Direction_Transmitter);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != SUCCESS) return;

    /* ���ʹ洢��ַ�����ֽڣ� */
    I2C_SendData(I2C1, (uint8_t)(addr >> 8));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;
    
    /* ���ʹ洢��ַ�����ֽڣ� */
    I2C_SendData(I2C1, (uint8_t)(addr & 0xFF));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;

    /* �������� */
    for (i = 0; i < write_len; i++) {
        I2C_SendData(I2C1, data[i]);
        if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;
    }

    /* ����ֹͣ�ź� */
    I2C_GenerateSTOP(I2C1, ENABLE);
    
    /* �ȴ�д������� */
    delay_ms(10);
}

/* ������ȡ����ֽ� */
void AT24C256_ReadBuffer(uint16_t addr, uint8_t *buffer, uint16_t len)
{
    uint16_t i;

    /* ������ʼ�ź� */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS) return;

    /* �����豸��ַ��дģʽ�� */
    I2C_Send7bitAddress(I2C1, AT24C256_ADDRESS, I2C_Direction_Transmitter);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != SUCCESS) return;

    /* ���ʹ洢��ַ�����ֽڣ� */
    I2C_SendData(I2C1, (uint8_t)(addr >> 8));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;
    
    /* ���ʹ洢��ַ�����ֽڣ� */
    I2C_SendData(I2C1, (uint8_t)(addr & 0xFF));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS) return;

    /* ���·�����ʼ�ź� */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS) return;

    /* �����豸��ַ����ģʽ�� */
    I2C_Send7bitAddress(I2C1, AT24C256_ADDRESS | 0x01, I2C_Direction_Receiver);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) != SUCCESS) return;

    /* ��ȡ���� */
    for (i = 0; i < len; i++) {
        /* ���һ���ֽڽ�ֹӦ�� */
        if (i == len - 1) {
            I2C_AcknowledgeConfig(I2C1, DISABLE);
            I2C_GenerateSTOP(I2C1, ENABLE);
        }

        if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_RECEIVED) != SUCCESS) return;
        buffer[i] = I2C_ReceiveData(I2C1);
    }

    /* ����ʹ��Ӧ�� */
    I2C_AcknowledgeConfig(I2C1, ENABLE);
}

///* ���Ժ��� */
//void AT24C256_Test(void)
//{
//    uint8_t test_data[64] = {0};
//    uint8_t read_data[64] = {0};
//    uint16_t i;
//    
//    /* ׼���������� */
//    for (i = 0; i < 64; i++) {
//        test_data[i] = i+2;
//    }
//    
//    /* ҳд����ԣ���ַ0x1000�� */
//    AT24C256_WritePage(0x1000, test_data, 64);
//    
//    /* ��ȡ��֤ */
//    AT24C256_ReadBuffer(0x1000, read_data, 64);
//    
//    /* ��֤��� */
//    for (i = 0; i < 64; i++) {
//        if (read_data[i] != test_data[i]) {
//			printf("\r\n AT24C256 init error \r\n");
//			break;
//        }
//		else
//		{
//			printf("\r\n AT24C256 init success \r\n");
//			for (i = 0; i < 64; i++) 
//			{
//				printf("\r\n test_data %d : %x\r\n",i,test_data[i]);
//			}
//			
//		}
//    }
//	printf("\r\n AT24C256 init ok \r\n");
//}

