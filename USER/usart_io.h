#ifndef __UART_IO__H_
#define __UART_IO__H_
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_exti.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

enum
{
	N_CMD, // ����
	W_CMD, // д
	R_CMD, // ��
	F_CMD, // ���
};
enum
{
	UNWAIT,
	WAITING,
};
// ����У��ģʽö��
typedef enum
{
    NO_PARITY,  // ��У��
    ODD_PARITY, // ��У��
    EVEN_PARITY // żУ��
} ParityMode;

// ����ֹͣλģʽö��
typedef enum
{
    None_STOP_BIT, // ��
    ONE_STOP_BIT,  // 1λֹͣλ
    TWO_STOP_BITS  // 2λֹͣλ
} StopBitMode;

#define U_TIM TIM4				// UART�ڲ���ʱ��
#define TX_GPIO_PIN GPIO_PIN_10 // UART��Ӧ��TX����
#define RX_GPIO_PIN GPIO_PIN_9	// UART��Ӧ��RX����
#define TX_PIN 14				// UART��Ӧ��TX���ű��
#define RX_PIN 15				// UART��Ӧ��RX���ű��
#define TX_GPIO GPIOB			// UART��Ӧ��TX�˿�
#define RX_GPIO GPIOB			// UART��Ӧ��RX�˿�

#define MAX_WAIT 0x5 // ���ȴ�ʱ��
#define T_RELOAD 68	 // ����һ���ֽ������ʱ��
#define T_FIRST 0x33 // ��ʱ����һλ��ʱ��

#define MAX_BUFSIZE 3000 // �շ�����ַ�����
uint8_t GetUartIOCounter(void);
#define DEBUG_SIMULATOR 0 // ���Դ��ڵĿ���

void USART_GPIO_Init(void);
void Uart_SendByte(uint8_t data);
uint8_t Uart_ReceiveByte(void);
void TIM3_IRQHandler(void);
void Timer3_Init(int BAUD_RATE, int USART_WordLength, int USART_Parity, int USART_StopBits);
#endif
