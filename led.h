#ifndef __LED__H__
#define __LED__H__
#include "stm32f10x.h"

#define CLOCK 72/8

void LED1_Config(void);
void LED1_On(void);
void LED1_Off(void);
void LED1_Flashing(unsigned int interval);
typedef void (*Func)(void);

typedef struct _SoftTimer{
	uint32_t m_nCounter;		//计数器
	uint32_t m_nMaxCounter;		//定时器
	Func funcCallBack;			//回调函数
}SoftTimer;

#endif
