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
#define COM_RX_HIGH() GPIO_SetBits(COM_RX_PORT, COM_RX_PIN)  // 高电平
#define COM_RX_LOW() GPIO_ResetBits(COM_RX_PORT, COM_RX_PIN) // 低电平

#define COM_TX_PORT GPIOB
#define COM_TX_PIN GPIO_Pin_6
#define COM_DATA_HIGH() GPIO_SetBits(COM_TX_PORT, COM_TX_PIN)  // 高电平
#define COM_DATA_LOW() GPIO_ResetBits(COM_TX_PORT, COM_TX_PIN) // 低电平

#define U_TIM TIM4              // UART内部定时器
#define TX_GPIO_PIN GPIO_PIN_10 // UART对应的TX引脚
#define RX_GPIO_PIN GPIO_PIN_9  // UART对应的RX引脚
#define TX_PIN 14               // UART对应的TX引脚编号
#define RX_PIN 15               // UART对应的RX引脚编号
#define TX_GPIO GPIOB           // UART对应的TX端口
#define RX_GPIO GPIOB           // UART对应的RX端口

#define DEBUG_SIMULATOR 0 // 调试串口的开关

// 定义循环队列容量的宏
#define QUEUE_SIZE 1024

// 定义循环队列结构体
typedef struct CircularQueue
{
    char data[QUEUE_SIZE]; // 存储数据的数组，使用宏定义的队列大小
    int front;             // 队头指针
    int rear;              // 队尾指针
    int size;              // 当前队列中元素的个数
} CircularQueue;

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
    ONE_STOP_BIT, // 1位停止位
    TWO_STOP_BITS // 2位停止位
} StopBitMode;

void USART_GPIO_Init(void); // 初始化GPIO
void TIM3_IRQHandler(void);
void Timer3_Init(void);
void Uart_IO_SendByte(uint8_t data);
void Uart_IO_SendByteStr(char *str, int len);
#endif
