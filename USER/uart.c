#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "uart.h"
#include "usart_io.h"

extern uint8_t UART1_RxBuffer[128];
extern uint8_t UART1_RxCount;
extern uint8_t UART1_ReceiveState;

extern uint8_t UART2_RxBuffer[128];
extern uint8_t UART2_RxCount;
extern uint8_t UART2_ReceiveState;

extern uint8_t UART3_RxBuffer[128];
extern uint8_t UART3_RxCount;
extern uint8_t UART3_ReceiveState;
volatile static ParityMode l_sParityMode = NO_PARITY;	   // ȫ�ֱ����������趨��ǰУ��ģʽ
volatile static StopBitMode l_sStopBitMode = ONE_STOP_BIT; // ȫ�ֱ����������趨��ǰֹͣλģʽ
volatile static int l_nUartWordLength = 8;

// ��������У�鲢������ֵ
int checkAndSetRate(int rate)
{
	int validRates[] = {2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200};
	int i;
	for (i = 0; i < sizeof(validRates) / sizeof(validRates[0]); i++)
	{
		if (rate == validRates[i])
		{
			return rate;
		}
	}
	LOG(LOG_ERR, "Invalid baud rate, using default 115200\n");
	return 115200;
}

/*******************************************************************************
 * Function Name : USART1_Config
 * Description   : ��ʼ������1
 * Return        : None
 *******************************************************************************/
void USART1_Config(int BAUD_RATE, int USART_WordLength, int USART_Parity, int USART_StopBits)
{
	int baud_rate = checkAndSetRate(BAUD_RATE);
	if (USART_WordLength < 7 || USART_WordLength > 9)
	{
		USART_WordLength = 8;
	}
	else
	{
		LOG(LOG_ERR, "Invalid USART_WordLength, using default 8\n");
	}
	if (USART_Parity < NO_PARITY || USART_Parity > EVEN_PARITY)
	{
		USART_Parity = NO_PARITY;
	}
	else
	{
		LOG(LOG_ERR, "Invalid USART_Parity, using default NO_PARITY\n");
	}
	if (USART_StopBits < ONE_STOP_BIT || USART_StopBits > TWO_STOP_BITS)
	{
		USART_StopBits = ONE_STOP_BIT;
	}
	else
	{
		LOG(LOG_ERR, "Invalid USART_StopBits, using default ONE_STOP_BIT\n");
	}

	l_nUartWordLength = USART_WordLength;		  // ������Чλ
	l_sParityMode = (ParityMode)USART_Parity;	  // ����У��ģʽ
	l_sStopBitMode = (StopBitMode)USART_StopBits; // ����ֹͣλģʽ

	// ����ṹ��
	USART_TypeDef *USARTx = USART1;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_ClockInitTypeDef USART_ClockInitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	// ����жϱ�ʶ
	USART_ClearFlag(USARTx, USART_FLAG_TC);
	// ��λ����x
	USART_DeInit(USARTx);
	// // ������ӳ��
	// GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);

	// ����TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	// ����TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// ����USART
	USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
	USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
	USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
	USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;
	USART_ClockInit(USARTx, &USART_ClockInitStructure);

	USART_InitStructure.USART_BaudRate = baud_rate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	if (l_nUartWordLength == 7)
	{
		// 7 N 1		8 N 0.5
		// 7 N 2		8 N 1
		// 7 O 1		8 N 1
		// 7 O 2		8 N 2
		// 7 E 1		8 N 1
		// 7 E 2		8 N 2
		if (l_sParityMode == NO_PARITY)
		{
			USART_InitStructure.USART_Parity = USART_Parity_No;
			// ����ֹͣλ
			if (l_sStopBitMode == ONE_STOP_BIT)
			{
				USART_InitStructure.USART_StopBits = USART_StopBits_0_5;
			}
			else
			{
				USART_InitStructure.USART_StopBits = USART_StopBits_1;
			}
		}
		else
		{
			USART_InitStructure.USART_Parity = USART_Parity_No;
			// ����ֹͣλ
			if (l_sStopBitMode == ONE_STOP_BIT)
			{
				USART_InitStructure.USART_StopBits = USART_StopBits_1;
			}
			else
			{
				USART_InitStructure.USART_StopBits = USART_StopBits_2;
			}
		}
	}
	else
	{
		// 8 N 1		8 N 1
		// 8 N 2		8 N 2
		// 8 O 1		8 O 1
		// 8 O 2		8 O 2
		// 8 E 1		8 E 1
		// 8 E 2		8 E 2
		//  ����У��λ
		if (l_sParityMode == NO_PARITY)
		{
			USART_InitStructure.USART_Parity = USART_Parity_No;
		}
		else if (l_sParityMode == ODD_PARITY)
		{
			USART_InitStructure.USART_Parity = USART_Parity_Odd;
		}
		else
		{
			USART_InitStructure.USART_Parity = USART_Parity_Even;
		}

		// ����ֹͣλ
		if (l_sStopBitMode == ONE_STOP_BIT)
		{
			USART_InitStructure.USART_StopBits = USART_StopBits_1;
		}
		else
		{
			USART_InitStructure.USART_StopBits = USART_StopBits_2;
		}
		// 9 N 1		9 N 1
		// 9 N 2		9 N 2
		// 9 O 1		9 O 1
		// 9 O 2		9 O 2
		// 9 E 1		9 E 1
		// 9 E 2		9 E 2
		if (l_nUartWordLength == 9)
		{
			USART_InitStructure.USART_WordLength = USART_WordLength_9b;
		}
	}
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USARTx, &USART_InitStructure);
	// �����ж�
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	if (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == SET)
	{
		USART_ClearFlag(USARTx, USART_FLAG_TC);
	}
	// ��ʼ������NVIC�Ĵ���
	NVIC_Init(&NVIC_InitStructure);
	// �򿪴��ڿ����ж� */
	USART_ITConfig(USARTx, USART_IT_IDLE, ENABLE);
	// �򿪴��ڽ����ж�
	USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
	USART_Cmd(USARTx, ENABLE);
}

/*******************************************************************************
 * Function Name : USART2_Config
 * Description   : ��ʼ������2
 * Return        : None
 *******************************************************************************/
void USART2_Config(void)
{
	// ����ṹ��
	USART_TypeDef *USARTx = USART2;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_ClockInitTypeDef USART_ClockInitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	// ����жϱ�ʶ
	USART_ClearFlag(USARTx, USART_FLAG_TC);
	// ��λ����
	USART_DeInit(USARTx);
	// ����TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	// ����TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	// ����USART
	USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
	USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
	USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
	USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;
	USART_ClockInit(USARTx, &USART_ClockInitStructure);
	#if PROJ_TYPE == PROJ_PACE1004
	USART_InitStructure.USART_BaudRate = 9600;
	#else
	USART_InitStructure.USART_BaudRate = 19200;
	#endif
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USARTx, &USART_InitStructure);
	// �����ж�
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	if (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == SET)
	{
		USART_ClearFlag(USARTx, USART_FLAG_TC);
	}
	// ��ʼ������NVIC�Ĵ���
	NVIC_Init(&NVIC_InitStructure);
	// �򿪴��ڿ����ж� */
	USART_ITConfig(USARTx, USART_IT_IDLE, ENABLE);
	// �򿪴��ڽ����ж�
	USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
	USART_Cmd(USARTx, ENABLE);
}

/*******************************************************************************
 * Function Name : USART3_Config
 * Description   : ��ʼ������3��RS485��
 * Return        : None
 *******************************************************************************/
void USART3_Config(void)
{
	// ����ṹ��
	USART_TypeDef *USARTx = USART3;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_ClockInitTypeDef USART_ClockInitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	// ����жϱ�ʶ
	USART_ClearFlag(USARTx, USART_FLAG_TC);
	// ��λ����
	USART_DeInit(USARTx);
	// ����485
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	Set485SendMode(); // ����Ϊ����ģʽ

	// ����TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	// ����RXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	// ����USART
	USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
	USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
	USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
	USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;
	USART_ClockInit(USARTx, &USART_ClockInitStructure);
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USARTx, &USART_InitStructure);
	// �����ж�
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	if (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == SET)
	{
		USART_ClearFlag(USARTx, USART_FLAG_TC);
	}
	// ��ʼ������NVIC�Ĵ���
	NVIC_Init(&NVIC_InitStructure);
	// �򿪴��ڿ����ж� */
	USART_ITConfig(USARTx, USART_IT_IDLE, ENABLE);
	// ʹ�ܴ��ڽ����ж�
	USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
	USART_Cmd(USARTx, ENABLE);
}
void Set485SendMode(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_12); // 485����ģʽ
}
void Set485ReceiveMode(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_12); // 485����ģʽ
}

/*******************************************************************************
 * Function Name : USART1_IRQHandler
 * Description   : ����1�жϷ������
 * Input         : None
 * Return        : None
 *******************************************************************************/
void USART1_IRQHandler(void)
{
	// ���ֶ��巽��������������������"û���õ�"����
	uint8_t Clear = Clear;
	// ������յ�����
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		uint8_t tmpValue = USART1->DR;
		if (l_nUartWordLength == 7)
		{ // �����7λ���ݣ���ôֻȡ��7λ����
			tmpValue &= 0x7F;
		}
		// �ѽ��յ����ֽڱ��棬�����ַ��1
		UART1_RxBuffer[UART1_RxCount++] = tmpValue;
	}
	// ������յ�1֡����
	else if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	{
		// ��SR�Ĵ���
		Clear = USART1->SR;
		// ��DR�Ĵ���(�ȶ�SR�ٶ�DR������Ϊ�����IDLE�ж�)
		Clear = USART1->DR;
		// ��ǽ��յ���1֡����
		UART1_ReceiveState = 1;
	}
}

/*******************************************************************************
 * Function Name : USART2_IRQHandler
 * Description   : ����2�жϷ������
 * Input         : None
 * Return        : None
 *******************************************************************************/
void USART2_IRQHandler(void)
{
	// ���ֶ��巽��������������������"û���õ�"����
	uint8_t Clear = Clear;
	// ������յ�����
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		// �ѽ��յ����ֽڱ��棬�����ַ��1
		UART2_RxBuffer[UART2_RxCount++] = USART2->DR;
	}
	// ������յ�1֡����
	else if (USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)
	{
		// ��SR�Ĵ���
		Clear = USART2->SR;
		// ��DR�Ĵ���(�ȶ�SR�ٶ�DR������Ϊ�����IDLE�ж�)
		Clear = USART2->DR;
		// ��ǽ��յ���1֡����
		UART2_ReceiveState = 1;
	}
}

/*******************************************************************************
 * Function Name : USART3_IRQHandler
 * Description   : ����3�жϷ������
 * Input         : None
 * Return        : None
 *******************************************************************************/
void USART3_IRQHandler(void)
{
	// ���ֶ��巽��������������������"û���õ�"����
	uint8_t Clear = Clear;
	// ������յ�����
	if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
	{
		// �ѽ��յ����ֽڱ��棬�����ַ��1
		UART3_RxBuffer[UART3_RxCount++] = USART3->DR;
	}
	// ������յ�1֡����
	else if (USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)
	{
		// ��SR�Ĵ���
		Clear = USART3->SR;
		// ��DR�Ĵ���(�ȶ�SR�ٶ�DR������Ϊ�����IDLE�ж�)
		Clear = USART3->DR;
		// ��ǽ��յ���1֡����
		UART3_ReceiveState = 1;
	}
}

// �������ڼ��㲢������żУ��λ
unsigned char setParityBit(unsigned char Data, ParityMode checkMode)
{
	unsigned char last7Bits = Data & 0x7F; // ��ȡData�����7λ
	unsigned char parityBit = 0;
	// �������7λ��1�ĸ���
	for (int i = 0; i < 7; i++)
	{
		parityBit ^= (last7Bits >> i) & 0x01;
	}

	switch (checkMode)
	{
	case NO_PARITY:
		break;
	case ODD_PARITY:
		if (parityBit == 0)
		{
			parityBit = 1;
		}
		break;
	case EVEN_PARITY:
		if (parityBit == 1)
		{
			parityBit = 0;
		}
		break;
	}

	Data &= 0x7F;			  // �����ԭ�������λ������ԭ�����λ������ֵ��
	Data |= (parityBit << 7); // ����żУ��λ�ŵ����λ
	return Data;
}

/*******************************************************************************
* Function Name : USART1_SendByte
* Description   : ��USART1����һ���ֽ�
									Data = Ҫ���͵�����
* Return        : None
*******************************************************************************/
void USART1_SendByte(uint16_t Data)
{
	if (l_nUartWordLength == 7)
	{ // �����7λ���ݣ���ôֻȡ��7λ����
		Data = setParityBit((unsigned char)Data, l_sParityMode);
	}
	USART_TypeDef *USARTx = USART1;
	USARTx->DR = (Data & (uint16_t)0x01FF);
	while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) != SET)
		;
}
int USART1_SendStr(char *str, uint8_t len)
{
	while (*str != '\0')
	{
		USART1_SendByte(*str++);
	}
	return 0;
}
/*******************************************************************************
* Function Name : USART1_SendByte
* Description   : ��USART1����һ���ֽ�
									Data = Ҫ���͵�����
* Return        : None
*******************************************************************************/
void USART2_SendByte(uint16_t Data)
{
	USART_TypeDef *USARTx = USART2;
	USARTx->DR = (Data & (uint16_t)0x01FF);
	while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) != SET)
		;
}

int USART2_SendStr(uint8_t *str, uint8_t len)
{
	while (*str != '\0')
	{
		USART2_SendByte(*str++);
	}
	return 0;
}

/*******************************************************************************
* Function Name : USART3_SendByte
* Description   : ��USART3����һ���ֽ�
									Data = Ҫ���͵�����
* Return        : None
*******************************************************************************/
void USART3_SendByte(uint16_t Data)
{
	Set485SendMode();
	USART_TypeDef *USARTx = USART3;
	USARTx->DR = (Data & (uint16_t)0x01FF);
	while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) != SET)
		;
	Set485ReceiveMode();
}

int USART3_SendStr(char *str, uint8_t len)
{
	USART_TypeDef *USARTx = USART3;
	Set485SendMode();
	while (*str != '\0')
	{
		uint16_t Data = *str++;
		USARTx->DR = (Data & (uint16_t)0x01FF);
		while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) != SET)
			;
		USART1_SendByte(*str++);
	}
	Set485ReceiveMode();
	return 0;
}

#include "stdio.h"
#if 1
#pragma import(__use_no_semihosting)
// ��׼����Ҫ��֧�ֺ���
struct __FILE
{
	int handle;
};

FILE __stdout;
/**
 * @brief ����_sys_exit()�Ա���ʹ�ð�����ģʽ
 * @param void
 * @return  void
 */
void _sys_exit(int x)
{
	x = x;
}

int fputc(int ch, FILE *f)
{
	Set485SendMode();
	USART_TypeDef *USARTx = USART3;
	USARTx->DR = (ch & (uint16_t)0x01FF);
	while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) != SET)
		;
	Set485ReceiveMode();
	return ch;
}
#endif
