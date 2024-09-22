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
* Description   : ��ʼ������1
* Return        : None 
*******************************************************************************/
void USART1_Config(void){
	//����ṹ��
	USART_TypeDef* USARTx=USART1;
	GPIO_InitTypeDef GPIO_InitStructure;                   
	USART_ClockInitTypeDef USART_ClockInitStructure;                               
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;
	//����жϱ�ʶ
	USART_ClearFlag(USARTx,USART_FLAG_TC);
	//��λ����x
	USART_DeInit(USARTx);
	 // ������ӳ��
    GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);

	// //����TXD
	// GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	// GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	// GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	// GPIO_Init(GPIOA,&GPIO_InitStructure);
	// //����TXD
	// GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	// GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	// GPIO_Init(GPIOA,&GPIO_InitStructure);

	//����TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	//����TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	//����USART
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
	//�����ж�
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;    
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	if(USART_GetFlagStatus(USARTx, USART_FLAG_TC)==SET){
    USART_ClearFlag(USARTx,USART_FLAG_TC);
	}
	//��ʼ������NVIC�Ĵ���
	NVIC_Init(&NVIC_InitStructure);
	//�򿪴��ڿ����ж� */
	USART_ITConfig(USARTx, USART_IT_IDLE, ENABLE);
	//�򿪴��ڽ����ж�
	USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
	USART_Cmd(USARTx,ENABLE);   
}

/*******************************************************************************
* Function Name : USART2_Config
* Description   : ��ʼ������2
* Return        : None 
*******************************************************************************/
void USART2_Config(void){
	//����ṹ��
	USART_TypeDef* USARTx=USART2;
	GPIO_InitTypeDef GPIO_InitStructure;                   
	USART_ClockInitTypeDef USART_ClockInitStructure;                               
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;
	//����жϱ�ʶ
	USART_ClearFlag(USARTx,USART_FLAG_TC);
	//��λ����
	USART_DeInit(USARTx);
	//����TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	//����TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	//����USART
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
	//�����ж�
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;    
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	if(USART_GetFlagStatus(USARTx, USART_FLAG_TC)==SET){
    USART_ClearFlag(USARTx,USART_FLAG_TC);
	}
	//��ʼ������NVIC�Ĵ���
	NVIC_Init(&NVIC_InitStructure);
	//�򿪴��ڿ����ж� */
	USART_ITConfig(USARTx, USART_IT_IDLE, ENABLE);
	//�򿪴��ڽ����ж�
	USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
	USART_Cmd(USARTx,ENABLE); 
}

/*******************************************************************************
* Function Name : USART3_Config
* Description   : ��ʼ������3��RS485��
* Return        : None 
*******************************************************************************/
void USART3_Config(void){
	//����ṹ��
	USART_TypeDef* USARTx=USART3;
	GPIO_InitTypeDef GPIO_InitStructure;                   
	USART_ClockInitTypeDef USART_ClockInitStructure;                               
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;
	//����жϱ�ʶ
	USART_ClearFlag(USARTx,USART_FLAG_TC);
	//��λ����
	USART_DeInit(USARTx);
	//����485
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	//����TXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	//����RXD
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	//����USART
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
	//�����ж�
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;    
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	if(USART_GetFlagStatus(USARTx, USART_FLAG_TC)==SET){
    USART_ClearFlag(USARTx,USART_FLAG_TC);
	}
	//��ʼ������NVIC�Ĵ���
	NVIC_Init(&NVIC_InitStructure);
	//ʹ�ܴ��ڽ����ж�
	USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
	USART_Cmd(USARTx,ENABLE);  
}

/*******************************************************************************
* Function Name : USART1_IRQHandler 
* Description   : ����1�жϷ������
* Input         : None
* Return        : None 
*******************************************************************************/
void USART1_IRQHandler(void)
{	
	//���ֶ��巽��������������������"û���õ�"����
	uint8_t Clear=Clear;
	//������յ�����
 	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		//�ѽ��յ����ֽڱ��棬�����ַ��1
		UART1_RxBuffer[UART1_RxCount++] = USART1->DR;
	}
	//������յ�1֡����
	else if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	{
		//��SR�Ĵ���
		Clear=USART1->SR;
		//��DR�Ĵ���(�ȶ�SR�ٶ�DR������Ϊ�����IDLE�ж�)
		Clear=USART1->DR;
		//��ǽ��յ���1֡����
		UART1_ReceiveState=1;
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
	//���ֶ��巽��������������������"û���õ�"����
	uint8_t Clear=Clear;
	//������յ�����
 	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		//�ѽ��յ����ֽڱ��棬�����ַ��1
		UART2_RxBuffer[UART2_RxCount++] = USART2->DR;
	}
	//������յ�1֡����
	else if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)
	{
		//��SR�Ĵ���
		Clear=USART2->SR;
		//��DR�Ĵ���(�ȶ�SR�ٶ�DR������Ϊ�����IDLE�ж�)
		Clear=USART2->DR;
		//��ǽ��յ���1֡����
		UART2_ReceiveState=1;
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
	//���ֶ��巽��������������������"û���õ�"����
	uint8_t Clear=Clear;
	//������յ�����
 	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
	{
		//�ѽ��յ����ֽڱ��棬�����ַ��1
		UART3_RxBuffer[UART3_RxCount++] = USART3->DR;
	}
	//������յ�1֡����
	else if(USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)
	{
		//��SR�Ĵ���
		Clear=USART3->SR;
		//��DR�Ĵ���(�ȶ�SR�ٶ�DR������Ϊ�����IDLE�ж�)
		Clear=USART3->DR;
		//��ǽ��յ���1֡����
		UART2_ReceiveState=1;
	}
}

/*******************************************************************************
* Function Name : USART1_SendByte 
* Description   : ��USART1����һ���ֽ�
									Data = Ҫ���͵�����
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
* Description   : ��USART1����һ���ֽ�
									Data = Ҫ���͵�����
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
* Description   : ��USART3����һ���ֽ�
									Data = Ҫ���͵�����
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
//��׼����Ҫ��֧�ֺ���
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
	USART_TypeDef* USARTx=USART3;
	USARTx->DR = (ch & (uint16_t)0x01FF);
	while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) != SET);
    return ch;
}
#endif