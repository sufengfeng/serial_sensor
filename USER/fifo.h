#ifndef _FIFO_H_
#define _FIFO_H_

#include "stm32f10x.h"

#define FIFO_SIZE 1024

typedef struct
{
	//定义缓冲区，大小用FIFO_SIZE定义
  uint8_t Buf[FIFO_SIZE];	   
	//定义读取指针
  volatile uint32_t Read;
	//定义写入指针
  volatile uint32_t Write;   
}FIFOTypeDef;
//串口1数据缓冲区
extern FIFOTypeDef FIFO1;
//串口2数据缓冲区
extern FIFOTypeDef FIFO2;
//临时数据缓冲区
extern FIFOTypeDef FIFOTmp;

void FIFO_Reset(FIFOTypeDef* FIFOx);
void FIFO_Push(FIFOTypeDef* FIFOx,uint8_t c);
uint8_t FIFO_Pop(FIFOTypeDef* FIFOx,uint8_t* c);
uint8_t FIFO_Get(FIFOTypeDef* FIFOx,uint8_t i,uint8_t* c);

#endif
