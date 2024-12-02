#ifndef __LED__H__
#define __LED__H__
#include "stm32f10x.h"

#define CLOCK 72/8

void BoardLED1_Config(void);
void BoardLED1_On(void);
void BoardLED1_Off(void);
void BoardLED1_Flashing(unsigned int interval);
typedef void (*Func)(void);

typedef struct _SoftTimer{
	uint32_t m_nCounter;		//计数�?
	uint32_t m_nMaxCounter;		//定时�?
	Func funcCallBack;			//回调函数
}SoftTimer;

#endif
