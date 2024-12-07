#include "fifo.h"
#include "timer.h"
#include "uart.h"
#include "usart_io.h"

/*******************************************************************************
 * Function Name : TIM2_Config
 * Description   : ��ʼ��TIM2
 * Input         : None
 * Return        : None
 *******************************************************************************/
void TIM2_Config(void) // ����1ms������
{
	// ����ṹ��
	TIM_TypeDef *TIMx = TIM2;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	// ��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ,������100Ϊ10ms
	TIM_TimeBaseStructure.TIM_Period = 1;
	// Ԥ��Ƶϵ��Ϊ36000-1������������ʱ��Ϊ72MHz/36000 = 2kHz
	TIM_TimeBaseStructure.TIM_Prescaler = 36000 - 1;
	// ����ʱ�ӷָ�
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	// TIM���ϼ���ģʽ
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	// ����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
	TIM_TimeBaseInit(TIMx, &TIM_TimeBaseStructure);
	// ʹ�ܻ���ʧ��ָ����TIM�ж�
	TIM_ITConfig(TIMx, TIM_IT_Update, ENABLE);
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	// ʹ��TIMx���� ��ʱ��ʹ��
	TIM_Cmd(TIMx, ENABLE);
}

/*******************************************************************************
 * Function Name : TIM2_IRQHandler
 * Description   : TIM2�жϷ������
 * Input         : None
 * Return        : None
 *******************************************************************************/
void TIM2_IRQHandler(void)
{
	// ����ṹ��
	TIM_TypeDef *TIMx = TIM2;
	// ���ָ����TIM�жϷ������:TIM �ж�Դ
	if (TIM_GetITStatus(TIMx, TIM_IT_Update) != RESET)
	{
		// Frame_Handler(&FIFO1,USART2);
		// Frame_Handler(&FIFO2,USART1);
		UpdataSoftTimer();
		// ���TIMx���жϴ�����λ:TIM �ж�Դ
		TIM_ClearITPendingBit(TIMx, TIM_IT_Update);
	};
}

// ����Flash��СΪ64KB
#define FLASH_SIZE (64 * 1024) // 64KB
#define FLASH_SECTOR_SIZE 1024 // ������С1KB
#define FLASH_LAST_SECTOR (FLASH_SIZE - FLASH_SECTOR_SIZE)
#define FLASH_LAST_SECTOR_ADDRESS (0x08000000 + FLASH_LAST_SECTOR)

// д��Flash�ĺ���
void Flash_Write_Data(uint32_t StartSectorAddress, uint8_t *data, uint16_t num_bytes)
{
	FLASH_Unlock(); // ����Flash

	// ������е�Flash��־
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

	// // ������һ�������������Ȳ���
	// if ((StartSectorAddress - FLASH_LAST_SECTOR_ADDRESS) + num_bytes > FLASH_SECTOR_SIZE) {
	//     FLASH_ErasePage(FLASH_LAST_SECTOR_ADDRESS);
	// }
	FLASH_ErasePage(FLASH_LAST_SECTOR_ADDRESS);
	// �԰���Ϊ��λ���б�̣�һ��������16λ��
	for (uint32_t i = 0; i < num_bytes; i += 2)
	{
		uint16_t data_to_write = data[i] | (data[i + 1] << 8);
		FLASH_ProgramHalfWord(StartSectorAddress + i, data_to_write);
	}

	FLASH_Lock(); // ����Flash
}

// ��Flash��ȡ���ݵĺ���
void Flash_Read_Data(uint32_t StartSectorAddress, uint8_t *data, uint16_t num_bytes)
{
	for (uint32_t i = 0; i < num_bytes; i++)
	{
		data[i] = *(__IO uint8_t *)(StartSectorAddress + i);
	}
}

// д��Ͷ�ȡ���ݵ�ʾ��
void Flash_Write_Read_Example(void)
{
	uint8_t write_data[128]; // Ҫд�������
	uint8_t read_data[128];	 // ��ȡ������

	// ���Ҫд�������
	for (uint16_t i = 0; i < 128; i++)
	{
		write_data[i] = 128 - i;
	}
	printf("Flash Write and Read Example Started\n");
	// д�����ݵ����һ�������Ŀ�ʼ��
	Flash_Write_Data(FLASH_LAST_SECTOR_ADDRESS, write_data, 128);

	// �����һ�������Ŀ�ʼ����ȡ����
	Flash_Read_Data(FLASH_LAST_SECTOR_ADDRESS, read_data, 128);

	// �������
	for (uint16_t i = 0; i < 128; i++)
	{
		if (read_data[i] != write_data[i])
		{
			// ������
			printf("Error: Data mismatch at address [0x%08X][0x%08X]\n", read_data[i], write_data[i]);
			break;
		}
	}
	printf("Flash Write and Read Example Completed\n");
}
void InvertUint8(unsigned char *dBuf, unsigned char *srcBuf)
{
	uint32_t i;
	unsigned char tmp[4];
	tmp[0] = 0;
	for (i = 0; i < 8U; i++)
	{
		if ((srcBuf[0] & (1U << (uint32_t)i)) != 0U)
		{
			uint32_t bmpValue = (7U - i);
			tmp[0] = tmp[0] | 1U << bmpValue;
		}
	}
	dBuf[0] = tmp[0];
	return;
}

void InvertUint16(unsigned short *dBuf, unsigned short *srcBuf)
{
	int i;
	unsigned short tmp[4];
	tmp[0] = 0;
	for (i = 0; i < 16; i++)
	{
		if ((srcBuf[0] & (1U << (uint32_t)i)) != 0U)
		{
			tmp[0] = tmp[0] | (1U << (15U - (uint32_t)i));
		}
	}
	dBuf[0] = tmp[0];
	return;
}
/****************************Info**********************************************
 * Name:    CRC-16/IBM          x16+x15+x2+1
 * Width:	16
 * Poly:    0x8005
 * Init:    0x0000
 * Refin:   True
 * Refout:  True
 * Xorout:  0x0000
 * Alias:   CRC-16,CRC-16/ARC,CRC-16/LHA
 *****************************************************************************/
unsigned short CRC32_IBM(unsigned char *data, unsigned int datalen)
{
	unsigned short wCRCin = 0x0000;
	unsigned short wCPoly = 0x8005;
	unsigned char wChar = 0;

	while (datalen-- > 0)
	{
		wChar = *(data++);
		InvertUint8(&wChar, &wChar);
		wCRCin = wCRCin ^ ((unsigned short)wChar << 8U);
		for (int i = 0; i < 8; i++)
		{
			if ((wCRCin & 0x8000U) != 0U)
			{
				wCRCin = (wCRCin << 1U) ^ wCPoly;
			}
			else
			{
				wCRCin = wCRCin << 1;
			}
		}
	}
	InvertUint16(&wCRCin, &wCRCin);
	return (wCRCin);
}
GlobalBasicParam g_sGlobalBasicParam = {0}; // ȫ�ֻ�������

// ��ȡ��ӡ�ȼ�
inline int GetLogLevel(void)
{
	if (g_sGlobalBasicParam.m_nLogLevel == 0U)
	{
		g_sGlobalBasicParam.m_nLogLevel = LOG_INFO;
	}
	return g_sGlobalBasicParam.m_nLogLevel;
}

// ���ô�ӡ�ȼ�
void SetLogLevel(int level)
{
	if ((level <= LOG_DEBUG) && (level >= LOG_EMERG))
	{
		g_sGlobalBasicParam.m_nLogLevel = level;
	}
}
// ��Ƭ��Ӳ����
void Reboot(void)
{
	__disable_irq();
	__set_FAULTMASK(1); // �ر����ж�
	NVIC_SystemReset(); // ����Ƭ������
	while (1)
	{
		;
	}
}
GlobalBasicParam *GetBasicParamHandle(void)
{
	return (GlobalBasicParam *)&g_sGlobalBasicParam;
}
// ��Flash�����������
static int SaveBasicParamTolash(GlobalBasicParam *p_sGlobalBasicParam)
{
	Flash_Write_Data(FLASH_LAST_SECTOR_ADDRESS, (uint8_t *)p_sGlobalBasicParam, sizeof(GlobalBasicParam) / sizeof(uint8_t));
	return 0;
}

// ���ò�����������ò���Ĭ�ϲ���
int SaveBasicParamDefault(GlobalBasicParam *p_sGlobalBasicParam)
{
	GlobalBasicParam basicParam = {
		.m_nHardVersion = 0x0a, // Ӳ���汾��
		.m_nBaudRate = 9600,	// ͨ�Ų�����
		.m_nWordLength = 8,		// ͨ��can������λ
		.m_nStopBits = 1,		// ͨ��can��ֹͣλ
		.m_nParity = NO_PARITY, // ͨ��can��У��λ
	};
	memset((char *)p_sGlobalBasicParam, 0, sizeof(GlobalBasicParam));

	strcpy((char *)basicParam.m_sSYMBOL, CONFIG_SYMBOL_BASE);

	memcpy(p_sGlobalBasicParam, &basicParam, sizeof(GlobalBasicParam));

	p_sGlobalBasicParam->checksum = CRC32_IBM((uint8_t *)p_sGlobalBasicParam, sizeof(GlobalBasicParam) - 4U); // 12:�����ļ�У��

	int iRet = SaveBasicParamTolash(p_sGlobalBasicParam);
	if (iRet != 0)
	{
		LOG(LOG_ERR, "SaveBasicParamTolash");
	}
	return 0;
}

// ���浱ǰ��������
int SaveCurrentBasicParam(void)
{
	GlobalBasicParam *p_sGlobalBasicParam = (void *)GetBasicParamHandle();
	p_sGlobalBasicParam->checksum = CRC32_IBM((uint8_t *)p_sGlobalBasicParam, sizeof(GlobalBasicParam) - 4U); // 12:�����ļ�У��

	int iRet = SaveBasicParamTolash(p_sGlobalBasicParam);
	if (iRet != 0)
	{
		LOG(LOG_ERR, "SaveCurrentBasicParam error");
	}
	return 0;
}
// ��Flash�����������
int LoadBasicParamFromFlash(GlobalBasicParam *p_sGlobalBasicParam)
{
	int iRet = -1;
	int8_t retry = 3; // �ض�����
	do
	{
		Flash_Read_Data(FLASH_LAST_SECTOR_ADDRESS, (uint8_t *)p_sGlobalBasicParam, sizeof(GlobalBasicParam) / sizeof(uint8_t));
		u16 checksum = CRC32_IBM((uint8_t *)p_sGlobalBasicParam, sizeof(GlobalBasicParam) - 4U);
		if (checksum != p_sGlobalBasicParam->checksum)
		{
			LOG(LOG_ERR, "Load GlobalBasicParam param Error[%d][%d]!", checksum, (unsigned int)p_sGlobalBasicParam->checksum);
		}
		else
		{
			LOG(LOG_NOTICE, "Load GlobalBasicParam param OK!");
			iRet = 0;
			break;
		}
	} while (retry-- > 0);

	// �������δ�ܳɹ���ȡ
	if (retry <= 0)
	{
		LOG(LOG_ERR, "retry too many,so Save GlobalBasicParam Default param!");
		iRet = SaveBasicParamDefault(p_sGlobalBasicParam);
	}
	PrintBasicParam(&g_sGlobalBasicParam); // ��ӡ��������
	return iRet;
}

// ������ܰ�ȫ��ز�������
void PrintBasicParam(GlobalBasicParam *p_sGlobalBasicParam)
{
	int i = 1;
	LOG(LOG_NOTICE, "Basic Param(%2d) m_nHardVersion 	=0x%08X ", i++, (unsigned int)p_sGlobalBasicParam->m_nHardVersion);
	LOG(LOG_NOTICE, "Basic Param(%2d) m_nBaudRate 	=%d ", i++, (unsigned int)p_sGlobalBasicParam->m_nBaudRate);
	LOG(LOG_NOTICE, "Basic Param(%2d) m_nWordLength	=%d ", i++, (unsigned int)p_sGlobalBasicParam->m_nWordLength);
	LOG(LOG_NOTICE, "Basic Param(%2d) m_nParity  	=%d ", i++, (unsigned int)p_sGlobalBasicParam->m_nParity);
	LOG(LOG_NOTICE, "Basic Param(%2d) m_nStopBits 	=%d ", i++, (unsigned int)p_sGlobalBasicParam->m_nStopBits);

	// LOG(LOG_NOTICE,"Basic Param(%2d) m_nCanNodeID =%d ", i++, (unsigned int )p_sGlobalBasicParam->m_nCanNodeID);
	// LOG(LOG_NOTICE,"Basic Param(%2d) m_nCanBaudRate =%d ", i++, (unsigned int )p_sGlobalBasicParam->m_nCanBaudRate);
	//	LOG(LOG_NOTICE,"Basic Param(%2d) m_nManual =%d ", i++, (unsigned int )p_sGlobalBasicParam->m_nManual);
}
uint32_t GetSoftVersion(const char *str)
{
	uint32_t hardVersion = 0, masterVersion = 0, FeatureVersion = 0;
	sscanf(str, "%d.%d.%d", &hardVersion, &masterVersion, &FeatureVersion);
	return (hardVersion << 24) + (masterVersion << 16) + FeatureVersion;
}
