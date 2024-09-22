#ifndef __UART_IO__H_
#define __UART_IO__H_
#include "stm32f10x.h"
#include "stdio.h"
#include "stdlib.h"
#include "stm32f10x_usart.h"
 
enum
{
	N_CMD,	//����
	W_CMD,	//д
	R_CMD,	//��
	F_CMD,	//���
};
enum
{
	UNWAIT,
	WAITING,
};
 
 
#define U_TIM		TIM4			//UART�ڲ���ʱ��
#define	TX_GPIO_PIN	GPIO_PIN_10		//UART��Ӧ��TX����
#define	RX_GPIO_PIN	GPIO_PIN_9		//UART��Ӧ��RX����
#define TX_PIN		14				//UART��Ӧ��TX���ű��
#define RX_PIN		15				//UART��Ӧ��RX���ű��
#define	TX_GPIO		GPIOB			//UART��Ӧ��TX�˿�
#define	RX_GPIO		GPIOB			//UART��Ӧ��RX�˿�
 
#define MAX_WAIT 	0x5		//���ȴ�ʱ��
#define T_RELOAD 	68		//����һ���ֽ������ʱ��
#define T_FIRST		0x33	//��ʱ����һλ��ʱ��
 
#define MAX_BUFSIZE	3000	//�շ�����ַ�����
 uint8_t GetUartIOCounter(void);
 
 

#endif