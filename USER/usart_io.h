#ifndef __UART_IO__H_
#define __UART_IO__H_
#include "stm32f10x.h"
#include "stdio.h"
#include "stdlib.h"
#include "stm32f10x_usart.h"
 
enum
{
	N_CMD,	//空闲
	W_CMD,	//写
	R_CMD,	//读
	F_CMD,	//完成
};
enum
{
	UNWAIT,
	WAITING,
};
 
 
#define U_TIM		TIM4			//UART内部定时器
#define	TX_GPIO_PIN	GPIO_PIN_10		//UART对应的TX引脚
#define	RX_GPIO_PIN	GPIO_PIN_9		//UART对应的RX引脚
#define TX_PIN		14				//UART对应的TX引脚编号
#define RX_PIN		15				//UART对应的RX引脚编号
#define	TX_GPIO		GPIOB			//UART对应的TX端口
#define	RX_GPIO		GPIOB			//UART对应的RX端口
 
#define MAX_WAIT 	0x5		//最大等待时间
#define T_RELOAD 	68		//发送一个字节所需的时间
#define T_FIRST		0x33	//延时到第一位的时间
 
#define MAX_BUFSIZE	3000	//收发最大字符数量
 uint8_t GetUartIOCounter(void);
 
 

#endif