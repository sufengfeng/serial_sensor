#ifndef __UART__H_
#define __UART__H_
#include "stm32f10x.h"
#include "stm32f10x_usart.h"

void USART1_Config(void);
void USART2_Config(void);
void USART1_IRQHandler(void);
void USART2_IRQHandler(void);
void USART_SendByte(USART_TypeDef* USARTx,uint16_t Data);

#endif
