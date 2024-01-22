#ifndef __TIMER__H__
#define __TIMER__H__
#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "fifo.h"

void TIM2_Config(void);
void TIM2_IRQHandler(void);
//void Frame_Handler(FIFOTypeDef* FIFOx,USART_TypeDef* USARTx);
void UpdataSoftTimer(void );
#endif
