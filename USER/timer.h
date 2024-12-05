#ifndef __TIMER__H__
#define __TIMER__H__
#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_flash.h"

#include "stdio.h"
#include "string.h"

#include "fifo.h"

void TIM2_Config(void);
void TIM2_IRQHandler(void);
// void Frame_Handler(FIFOTypeDef* FIFOx,USART_TypeDef* USARTx);
void UpdataSoftTimer(void);

// 全局基本参数
typedef struct __GlobalBasicParam
{
    uint8_t m_sSYMBOL[4];    // 1:"CONFIG_SYMBOL_BASE"字节头
    uint32_t m_nHardVersion; // 2:硬件版本 0xA200/0xA300

    uint32_t m_nBaudRate;    // 3:波特率 4800-115200
    uint8_t m_nWordLength;   // 4:数据位     7/8
    uint8_t m_nStopBits;     // 5:停止位     1/2

    uint8_t m_nParity;    // 6:校验位       0/1/2
    uint8_t m_nLogLevel;   // 7:日志等级
    uint16_t reverse02;  // 8:保留字段
    uint32_t m_nAppVersion; // 2:硬件版本 0xA200/0xA300
    uint32_t reserved[16 - 6]; // 12:保留字段, 127-前面字段
    uint32_t checksum;         // 13:配置文件校验
} GlobalBasicParam, *p_GlobalBasicParam;


#define CONFIG_SYMBOL_BASE 		("BAS")			//基本配置参数头校验
void PrintBasicParam(GlobalBasicParam*p_sGlobalBasicParam);
void Reboot(void) ;

int  SaveCurrentBasicParam(void);
GlobalBasicParam *GetBasicParamHandle(void);
int LoadBasicParamFromFlash(GlobalBasicParam *p_sGlobalBasicParam);
#endif
