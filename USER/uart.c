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
volatile static ParityMode l_sParityMode = NO_PARITY;	   // 全局变量，用于设定当前校验模式
volatile static StopBitMode l_sStopBitMode = ONE_STOP_BIT; // 全局变量，用于设定当前停止位模式
volatile static int l_nUartWordLength = 8;

// 函数用于校验并设置数值
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
 * Description   : 初始化串口1
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

	l_nUartWordLength = USART_WordLength;		  // 设置有效位
	l_sParityMode = (ParityMode)USART_Parity;	  // 设置校验模式
	l_sStopBitMode = (StopBitMode)USART_StopBits; // 设置停止位模式

	// 定义结构体
	USART_TypeDef *USARTx = USART1;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_ClockInitTypeDef USART_ClockInitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	// 清除中断标识
	USART_ClearFlag(USARTx, USART_FLAG_TC);
	// 复位串口x
	USART_DeInit(USARTx);
	// // 配置重映射
	// GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);

	// 配置TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	// 配置TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// 配置USART
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
			// 配置停止位
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
			// 配置停止位
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
		//  配置校验位
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

		// 配置停止位
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
	// 配置中断
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	if (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == SET)
	{
		USART_ClearFlag(USARTx, USART_FLAG_TC);
	}
	// 初始化外设NVIC寄存器
	NVIC_Init(&NVIC_InitStructure);
	// 打开串口空闲中断 */
	USART_ITConfig(USARTx, USART_IT_IDLE, ENABLE);
	// 打开串口接收中断
	USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
	USART_Cmd(USARTx, ENABLE);
}

/*******************************************************************************
 * Function Name : USART2_Config
 * Description   : 初始化串口2
 * Return        : None
 *******************************************************************************/
void USART2_Config(void)
{
	// 定义结构体
	USART_TypeDef *USARTx = USART2;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_ClockInitTypeDef USART_ClockInitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	// 清除中断标识
	USART_ClearFlag(USARTx, USART_FLAG_TC);
	// 复位串口
	USART_DeInit(USARTx);
	// 配置TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	// 配置TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	// 配置USART
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
	// 配置中断
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	if (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == SET)
	{
		USART_ClearFlag(USARTx, USART_FLAG_TC);
	}
	// 初始化外设NVIC寄存器
	NVIC_Init(&NVIC_InitStructure);
	// 打开串口空闲中断 */
	USART_ITConfig(USARTx, USART_IT_IDLE, ENABLE);
	// 打开串口接收中断
	USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
	USART_Cmd(USARTx, ENABLE);
}

/*******************************************************************************
 * Function Name : USART3_Config
 * Description   : 初始化串口3（RS485）
 * Return        : None
 *******************************************************************************/
void USART3_Config(void)
{
	// 定义结构体
	USART_TypeDef *USARTx = USART3;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_ClockInitTypeDef USART_ClockInitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	// 清除中断标识
	USART_ClearFlag(USARTx, USART_FLAG_TC);
	// 复位串口
	USART_DeInit(USARTx);
	// 配置485
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	Set485SendMode(); // 设置为发送模式

	// 配置TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	// 配置RXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	// 配置USART
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
	// 配置中断
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	if (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == SET)
	{
		USART_ClearFlag(USARTx, USART_FLAG_TC);
	}
	// 初始化外设NVIC寄存器
	NVIC_Init(&NVIC_InitStructure);
	// 打开串口空闲中断 */
	USART_ITConfig(USARTx, USART_IT_IDLE, ENABLE);
	// 使能串口接收中断
	USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
	USART_Cmd(USARTx, ENABLE);
}
void Set485SendMode(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_12); // 485发送模式
}
void Set485ReceiveMode(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_12); // 485接收模式
}

/*******************************************************************************
 * Function Name : USART1_IRQHandler
 * Description   : 串口1中断服务程序
 * Input         : None
 * Return        : None
 *******************************************************************************/
void USART1_IRQHandler(void)
{
	// 这种定义方法，用来消除编译器的"没有用到"提醒
	uint8_t Clear = Clear;
	// 如果接收到数据
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		uint8_t tmpValue = USART1->DR;
		if (l_nUartWordLength == 7)
		{ // 如果是7位数据，那么只取低7位数据
			tmpValue &= 0x7F;
		}
		// 把接收到的字节保存，数组地址加1
		UART1_RxBuffer[UART1_RxCount++] = tmpValue;
	}
	// 如果接收到1帧数据
	else if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	{
		// 读SR寄存器
		Clear = USART1->SR;
		// 读DR寄存器(先读SR再读DR，就是为了清除IDLE中断)
		Clear = USART1->DR;
		// 标记接收到了1帧数据
		UART1_ReceiveState = 1;
	}
}

/*******************************************************************************
 * Function Name : USART2_IRQHandler
 * Description   : 串口2中断服务程序
 * Input         : None
 * Return        : None
 *******************************************************************************/
void USART2_IRQHandler(void)
{
	// 这种定义方法，用来消除编译器的"没有用到"提醒
	uint8_t Clear = Clear;
	// 如果接收到数据
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		// 把接收到的字节保存，数组地址加1
		UART2_RxBuffer[UART2_RxCount++] = USART2->DR;
	}
	// 如果接收到1帧数据
	else if (USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)
	{
		// 读SR寄存器
		Clear = USART2->SR;
		// 读DR寄存器(先读SR再读DR，就是为了清除IDLE中断)
		Clear = USART2->DR;
		// 标记接收到了1帧数据
		UART2_ReceiveState = 1;
	}
}

/*******************************************************************************
 * Function Name : USART3_IRQHandler
 * Description   : 串口3中断服务程序
 * Input         : None
 * Return        : None
 *******************************************************************************/
void USART3_IRQHandler(void)
{
	// 这种定义方法，用来消除编译器的"没有用到"提醒
	uint8_t Clear = Clear;
	// 如果接收到数据
	if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
	{
		// 把接收到的字节保存，数组地址加1
		UART3_RxBuffer[UART3_RxCount++] = USART3->DR;
	}
	// 如果接收到1帧数据
	else if (USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)
	{
		// 读SR寄存器
		Clear = USART3->SR;
		// 读DR寄存器(先读SR再读DR，就是为了清除IDLE中断)
		Clear = USART3->DR;
		// 标记接收到了1帧数据
		UART3_ReceiveState = 1;
	}
}

// 函数用于计算并设置奇偶校验位
unsigned char setParityBit(unsigned char Data, ParityMode checkMode)
{
	unsigned char last7Bits = Data & 0x7F; // 获取Data的最后7位
	unsigned char parityBit = 0;
	// 计算最后7位中1的个数
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

	Data &= 0x7F;			  // 先清除原来的最高位（假设原来最高位可能有值）
	Data |= (parityBit << 7); // 将奇偶校验位放到最高位
	return Data;
}

/*******************************************************************************
* Function Name : USART1_SendByte
* Description   : 向USART1发送一个字节
									Data = 要发送的数据
* Return        : None
*******************************************************************************/
void USART1_SendByte(uint16_t Data)
{
	if (l_nUartWordLength == 7)
	{ // 如果是7位数据，那么只取低7位数据
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
* Description   : 向USART1发送一个字节
									Data = 要发送的数据
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
* Description   : 向USART3发送一个字节
									Data = 要发送的数据
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
// 标准库需要的支持函数
struct __FILE
{
	int handle;
};

FILE __stdout;
/**
 * @brief 定义_sys_exit()以避免使用半主机模式
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
