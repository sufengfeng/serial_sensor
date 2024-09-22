#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "uart.h"

extern uint8_t UART1_RxBuffer[128];
extern uint8_t UART1_RxCount;
extern uint8_t UART1_ReceiveState;

extern uint8_t UART2_RxBuffer[128];
extern uint8_t UART2_RxCount;
extern uint8_t UART2_ReceiveState;

extern uint8_t UART3_RxBuffer[128];
extern uint8_t UART3_RxCount;
extern uint8_t UART3_ReceiveState;

/*******************************************************************************
* Function Name : USART1_Config
* Description   : 初始化串口1
* Return        : None 
*******************************************************************************/
void USART1_Config(void){
	//定义结构体
	USART_TypeDef* USARTx=USART1;
	GPIO_InitTypeDef GPIO_InitStructure;                   
	USART_ClockInitTypeDef USART_ClockInitStructure;                               
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;
	//清除中断标识
	USART_ClearFlag(USARTx,USART_FLAG_TC);
	//复位串口x
	USART_DeInit(USARTx);
	 // 配置重映射
    GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);

	// //配置TXD
	// GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	// GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	// GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	// GPIO_Init(GPIOA,&GPIO_InitStructure);
	// //配置TXD
	// GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	// GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	// GPIO_Init(GPIOA,&GPIO_InitStructure);

	//配置TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	//配置TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	//配置USART
	USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
	USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
	USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
	USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;
	USART_ClockInit(USARTx,&USART_ClockInitStructure);
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USARTx,&USART_InitStructure);
	//配置中断
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;    
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	if(USART_GetFlagStatus(USARTx, USART_FLAG_TC)==SET){
    USART_ClearFlag(USARTx,USART_FLAG_TC);
	}
	//初始化外设NVIC寄存器
	NVIC_Init(&NVIC_InitStructure);
	//打开串口空闲中断 */
	USART_ITConfig(USARTx, USART_IT_IDLE, ENABLE);
	//打开串口接收中断
	USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
	USART_Cmd(USARTx,ENABLE);   
}

/*******************************************************************************
* Function Name : USART2_Config
* Description   : 初始化串口2
* Return        : None 
*******************************************************************************/
void USART2_Config(void){
	//定义结构体
	USART_TypeDef* USARTx=USART2;
	GPIO_InitTypeDef GPIO_InitStructure;                   
	USART_ClockInitTypeDef USART_ClockInitStructure;                               
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;
	//清除中断标识
	USART_ClearFlag(USARTx,USART_FLAG_TC);
	//复位串口
	USART_DeInit(USARTx);
	//配置TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	//配置TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	//配置USART
	USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
	USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
	USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
	USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;
	USART_ClockInit(USARTx,&USART_ClockInitStructure);
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USARTx,&USART_InitStructure);
	//配置中断
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;    
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	if(USART_GetFlagStatus(USARTx, USART_FLAG_TC)==SET){
    USART_ClearFlag(USARTx,USART_FLAG_TC);
	}
	//初始化外设NVIC寄存器
	NVIC_Init(&NVIC_InitStructure);
	//打开串口空闲中断 */
	USART_ITConfig(USARTx, USART_IT_IDLE, ENABLE);
	//打开串口接收中断
	USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
	USART_Cmd(USARTx,ENABLE); 
}

/*******************************************************************************
* Function Name : USART3_Config
* Description   : 初始化串口3（RS485）
* Return        : None 
*******************************************************************************/
void USART3_Config(void){
	//定义结构体
	USART_TypeDef* USARTx=USART3;
	GPIO_InitTypeDef GPIO_InitStructure;                   
	USART_ClockInitTypeDef USART_ClockInitStructure;                               
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;
	//清除中断标识
	USART_ClearFlag(USARTx,USART_FLAG_TC);
	//复位串口
	USART_DeInit(USARTx);
	//配置485
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	//配置TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	//配置RXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	//配置USART
	USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
	USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
	USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
	USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;
	USART_ClockInit(USARTx,&USART_ClockInitStructure);
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USARTx,&USART_InitStructure);
	//配置中断
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;    
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	if(USART_GetFlagStatus(USARTx, USART_FLAG_TC)==SET){
    USART_ClearFlag(USARTx,USART_FLAG_TC);
	}
	//初始化外设NVIC寄存器
	NVIC_Init(&NVIC_InitStructure);
	//使能串口接收中断
	USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
	USART_Cmd(USARTx,ENABLE);  
}

/*******************************************************************************
* Function Name : USART1_IRQHandler 
* Description   : 串口1中断服务程序
* Input         : None
* Return        : None 
*******************************************************************************/
void USART1_IRQHandler(void)
{	
	//这种定义方法，用来消除编译器的"没有用到"提醒
	uint8_t Clear=Clear;
	//如果接收到数据
 	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		//把接收到的字节保存，数组地址加1
		UART1_RxBuffer[UART1_RxCount++] = USART1->DR;
	}
	//如果接收到1帧数据
	else if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	{
		//读SR寄存器
		Clear=USART1->SR;
		//读DR寄存器(先读SR再读DR，就是为了清除IDLE中断)
		Clear=USART1->DR;
		//标记接收到了1帧数据
		UART1_ReceiveState=1;
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
	//这种定义方法，用来消除编译器的"没有用到"提醒
	uint8_t Clear=Clear;
	//如果接收到数据
 	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		//把接收到的字节保存，数组地址加1
		UART2_RxBuffer[UART2_RxCount++] = USART2->DR;
	}
	//如果接收到1帧数据
	else if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)
	{
		//读SR寄存器
		Clear=USART2->SR;
		//读DR寄存器(先读SR再读DR，就是为了清除IDLE中断)
		Clear=USART2->DR;
		//标记接收到了1帧数据
		UART2_ReceiveState=1;
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
	//这种定义方法，用来消除编译器的"没有用到"提醒
	uint8_t Clear=Clear;
	//如果接收到数据
 	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
	{
		//把接收到的字节保存，数组地址加1
		UART3_RxBuffer[UART3_RxCount++] = USART3->DR;
	}
	//如果接收到1帧数据
	else if(USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)
	{
		//读SR寄存器
		Clear=USART3->SR;
		//读DR寄存器(先读SR再读DR，就是为了清除IDLE中断)
		Clear=USART3->DR;
		//标记接收到了1帧数据
		UART2_ReceiveState=1;
	}
}

/*******************************************************************************
* Function Name : USART1_SendByte 
* Description   : 向USART1发送一个字节
									Data = 要发送的数据
* Return        : None 
*******************************************************************************/
void USART1_SendByte(uint16_t Data)
{
	USART_TypeDef* USARTx=USART1;
	USARTx->DR = (Data & (uint16_t)0x01FF);
	while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) != SET);
}

/*******************************************************************************
* Function Name : USART1_SendByte 
* Description   : 向USART1发送一个字节
									Data = 要发送的数据
* Return        : None 
*******************************************************************************/
void USART2_SendByte(uint16_t Data)
{
	USART_TypeDef* USARTx=USART2;
	USARTx->DR = (Data & (uint16_t)0x01FF);
	while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) != SET);
}

/*******************************************************************************
* Function Name : USART3_SendByte 
* Description   : 向USART3发送一个字节
									Data = 要发送的数据
* Return        : None 
*******************************************************************************/
void USART3_SendByte(uint16_t Data)
{
	USART_TypeDef* USARTx=USART3;
	USARTx->DR = (Data & (uint16_t)0x01FF);
	while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) != SET);
}

#include "stdio.h"
#if 1
#pragma import(__use_no_semihosting)
//标准库需要的支持函数
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
	USART_TypeDef* USARTx=USART3;
	USARTx->DR = (ch & (uint16_t)0x01FF);
	while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) != SET);
    return ch;
}
#endif