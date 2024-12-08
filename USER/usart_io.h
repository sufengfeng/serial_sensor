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
    ONE_STOP_BIT,  // 1λֹͣλ
    TWO_STOP_BITS  // 2λֹͣλ
} StopBitMode;
enum{

COM_START_BIT, //��ʼλ

COM_D0_BIT, //bit0

COM_D1_BIT, //bit1

COM_D2_BIT, //bit2

COM_D3_BIT, //bit3

COM_D4_BIT, //bit4

COM_D5_BIT, //bit5

COM_D6_BIT, //bit6

COM_D7_BIT, //bit7

COM_STOP_BIT, //ֹͣλ

};
#define COM_RX_PORT GPIOB
#define COM_RX_PIN GPIO_Pin_7
#define COM_RX_STAT GPIO_ReadInputDataBit(COM_RX_PORT, COM_RX_PIN)
#define COM_RX_HIGH() GPIO_SetBits(COM_RX_PORT, COM_RX_PIN) //�ߵ�ƽ
#define COM_RX_LOW() GPIO_ResetBits(COM_RX_PORT, COM_RX_PIN) //�͵�ƽ


#define COM_TX_PORT GPIOB
#define COM_TX_PIN GPIO_Pin_6
#define COM_DATA_HIGH() GPIO_SetBits(COM_TX_PORT, COM_TX_PIN) //�ߵ�ƽ
#define COM_DATA_LOW() GPIO_ResetBits(COM_TX_PORT, COM_TX_PIN) //�͵�ƽ
#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int

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
void Timer3_Init(void);
void Uart_IO_SendByteStr(char *str, int len);
void Uart_IO_SendByte(uint8_t data);

// ����ѭ�����������ĺ�
#define QUEUE_SIZE 1024

// ����ѭ�����нṹ��
typedef struct CircularQueue {
    char data[QUEUE_SIZE];  // �洢���ݵ����飬ʹ�ú궨��Ķ��д�С
    int front;              // ��ͷָ��
    int rear;               // ��βָ��
    int size;               // ��ǰ������Ԫ�صĸ���
} CircularQueue;


#endif
