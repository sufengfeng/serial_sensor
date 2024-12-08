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
	N_CMD, // 空闲
	W_CMD, // 写
	R_CMD, // 读
	F_CMD, // 完成
};
enum
{
	UNWAIT,
	WAITING,
};
// 定义校验模式枚举
typedef enum
{
    NO_PARITY,  // 无校验
    ODD_PARITY, // 奇校验
    EVEN_PARITY // 偶校验
} ParityMode;

// 定义停止位模式枚举
typedef enum
{
    ONE_STOP_BIT,  // 1位停止位
    TWO_STOP_BITS  // 2位停止位
} StopBitMode;
enum{

COM_START_BIT, //起始位

COM_D0_BIT, //bit0

COM_D1_BIT, //bit1

COM_D2_BIT, //bit2

COM_D3_BIT, //bit3

COM_D4_BIT, //bit4

COM_D5_BIT, //bit5

COM_D6_BIT, //bit6

COM_D7_BIT, //bit7

COM_STOP_BIT, //停止位

};
#define COM_RX_PORT GPIOB
#define COM_RX_PIN GPIO_Pin_7
#define COM_RX_STAT GPIO_ReadInputDataBit(COM_RX_PORT, COM_RX_PIN)
#define COM_RX_HIGH() GPIO_SetBits(COM_RX_PORT, COM_RX_PIN) //高电平
#define COM_RX_LOW() GPIO_ResetBits(COM_RX_PORT, COM_RX_PIN) //低电平


#define COM_TX_PORT GPIOB
#define COM_TX_PIN GPIO_Pin_6
#define COM_DATA_HIGH() GPIO_SetBits(COM_TX_PORT, COM_TX_PIN) //高电平
#define COM_DATA_LOW() GPIO_ResetBits(COM_TX_PORT, COM_TX_PIN) //低电平
#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int

#define U_TIM TIM4				// UART内部定时器
#define TX_GPIO_PIN GPIO_PIN_10 // UART对应的TX引脚
#define RX_GPIO_PIN GPIO_PIN_9	// UART对应的RX引脚
#define TX_PIN 14				// UART对应的TX引脚编号
#define RX_PIN 15				// UART对应的RX引脚编号
#define TX_GPIO GPIOB			// UART对应的TX端口
#define RX_GPIO GPIOB			// UART对应的RX端口

#define MAX_WAIT 0x5 // 最大等待时间
#define T_RELOAD 68	 // 发送一个字节所需的时间
#define T_FIRST 0x33 // 延时到第一位的时间

#define MAX_BUFSIZE 3000 // 收发最大字符数量
uint8_t GetUartIOCounter(void);
#define DEBUG_SIMULATOR 0 // 调试串口的开关

void USART_GPIO_Init(void);
void Uart_SendByte(uint8_t data);
uint8_t Uart_ReceiveByte(void);
void TIM3_IRQHandler(void);
void Timer3_Init(void);
void Uart_IO_SendByteStr(char *str, int len);
void Uart_IO_SendByte(uint8_t data);

// 定义循环队列容量的宏
#define QUEUE_SIZE 1024

// 定义循环队列结构体
typedef struct CircularQueue {
    char data[QUEUE_SIZE];  // 存储数据的数组，使用宏定义的队列大小
    int front;              // 队头指针
    int rear;               // 队尾指针
    int size;               // 当前队列中元素的个数
} CircularQueue;


#endif
