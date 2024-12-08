#ifndef __UART_IO__H_
#define __UART_IO__H_
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_exti.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define COM_RX_PORT GPIOB
#define COM_RX_PIN GPIO_Pin_7
#define COM_RX_STAT GPIO_ReadInputDataBit(COM_RX_PORT, COM_RX_PIN)
#define COM_RX_HIGH() GPIO_SetBits(COM_RX_PORT, COM_RX_PIN)  // �ߵ�ƽ
#define COM_RX_LOW() GPIO_ResetBits(COM_RX_PORT, COM_RX_PIN) // �͵�ƽ

#define COM_TX_PORT GPIOB
#define COM_TX_PIN GPIO_Pin_6
#define COM_DATA_HIGH() GPIO_SetBits(COM_TX_PORT, COM_TX_PIN)  // �ߵ�ƽ
#define COM_DATA_LOW() GPIO_ResetBits(COM_TX_PORT, COM_TX_PIN) // �͵�ƽ

#define U_TIM TIM4              // UART�ڲ���ʱ��
#define TX_GPIO_PIN GPIO_PIN_10 // UART��Ӧ��TX����
#define RX_GPIO_PIN GPIO_PIN_9  // UART��Ӧ��RX����
#define TX_PIN 14               // UART��Ӧ��TX���ű��
#define RX_PIN 15               // UART��Ӧ��RX���ű��
#define TX_GPIO GPIOB           // UART��Ӧ��TX�˿�
#define RX_GPIO GPIOB           // UART��Ӧ��RX�˿�

#define DEBUG_SIMULATOR 0 // ���Դ��ڵĿ���

// ����ѭ�����������ĺ�
#define QUEUE_SIZE 1024

// ����ѭ�����нṹ��
typedef struct CircularQueue
{
    char data[QUEUE_SIZE]; // �洢���ݵ����飬ʹ�ú궨��Ķ��д�С
    int front;             // ��ͷָ��
    int rear;              // ��βָ��
    int size;              // ��ǰ������Ԫ�صĸ���
} CircularQueue;

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
    ONE_STOP_BIT, // 1λֹͣλ
    TWO_STOP_BITS // 2λֹͣλ
} StopBitMode;

void USART_GPIO_Init(void); // ��ʼ��GPIO
void TIM3_IRQHandler(void);
void Timer3_Init(void);
void Uart_IO_SendByte(uint8_t data);
void Uart_IO_SendByteStr(char *str, int len);
#endif
