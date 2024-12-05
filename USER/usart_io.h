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
    None_STOP_BIT, // 无
    ONE_STOP_BIT,  // 1位停止位
    TWO_STOP_BITS  // 2位停止位
} StopBitMode;

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
void Timer3_Init(int BAUD_RATE, int USART_WordLength, int USART_Parity, int USART_StopBits);
#endif
