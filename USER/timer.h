#ifndef __TIMER__H__
#define __TIMER__H__
#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "fifo.h"

void TIM2_Config(void);
void TIM2_IRQHandler(void);
// void Frame_Handler(FIFOTypeDef* FIFOx,USART_TypeDef* USARTx);
void UpdataSoftTimer(void);

// ȫ�ֻ�������
typedef struct __GlobalBasicParam
{
    uint8_t m_sSYMBOL[4];    // 1:"CONFIG_SYMBOL_BASE"�ֽ�ͷ
    uint32_t m_nHardVersion; // 2:Ӳ���汾 0xA200/0xA300

    uint16_t m_nBaudRate;    // 3:������
    uint8_t m_nWordLength;   // 4:����λ
    uint8_t m_nStopBits;     // 5:ֹͣλ

    uint8_t m_nParity;    // 6:У��λ
    uint8_t m_nLogLevel;   // 7:��־�ȼ�
    uint16_t reverse02;  // 8:�����ֶ�

    uint32_t reserved[32 - 5]; // 12:�����ֶ�, 127-ǰ���ֶ�
    uint32_t checksum;         // 13:�����ļ�У��
} GlobalBasicParam, *p_GlobalBasicParam;

//�豸����		�豸���Ͳ�ͬ�����������Ͳ�ͬ
typedef enum
{
	enumNone,   //��
    enumOdd,    //��У��
    enumEven,    //żУ��
}enumParity;
#define CONFIG_SYMBOL_BASE 		("BAS")			//�������ò���ͷУ��

#endif
