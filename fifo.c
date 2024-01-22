#include "stm32f10x.h"
#include "fifo.h"

FIFOTypeDef FIFO1;
FIFOTypeDef FIFO2;
FIFOTypeDef FIFOTmp;

/*******************************************************************************
* Function Name : FIFO_Reset
* Description   : 重置FIFOx缓冲区
* Input         : FIFOx
* Output        : None
* Return        : None
*******************************************************************************/
void FIFO_Reset(FIFOTypeDef* FIFOx)
{
    FIFOx->Write = 0;
    FIFOx->Read = 0;
}

/*******************************************************************************
* Function Name : FIFO_PutChar
* Description   : 将数据写入到FIFOx的缓冲区（入栈）
* Input         : c
* Output        : None
* Return        : None
*******************************************************************************/
void FIFO_Push(FIFOTypeDef* FIFOx,uint8_t c)
{
	//写入数据到缓冲区
  FIFOx->Buf[FIFOx->Write] = c;  
  //写入指针超出缓冲区最大值，返回最初位置
  if(++FIFOx->Write >= FIFO_SIZE)	   
  {
    FIFOx->Write = 0;
  }
}

/*******************************************************************************
* Function Name  : FIFO_GetChar
* Description    : 从FIFO缓冲区读取数据（出栈）
* Input          : c
* Output         : None
* Return         : 0 
*******************************************************************************/
uint8_t FIFO_Pop(FIFOTypeDef* FIFOx,uint8_t* c)
{
	//如果没有存入数据，则返回0
  if(FIFOx->Read == FIFOx->Write)	  
  {
    return 0;
  }
  else
  {
		//读取数据，传入到指针c
    *c = FIFOx->Buf[FIFOx->Read];	 
    //读取指针超出缓冲区最大值，返回最初位置
    if (++FIFOx->Read >= FIFO_SIZE)	 
    {
      FIFOx->Read = 0;
    }
		//成功读取数据返回1
    return 1;	
  }
}

/*******************************************************************************
* Function Name  : FIFO_GetChar
* Description    : 获取FIFO缓冲区指定索引的数据（仅读取）
* Input          : c（输出的字节）,i（index，索引）
* Output         : None
* Return         : 0 
*******************************************************************************/
uint8_t FIFO_Get(FIFOTypeDef* FIFOx,uint8_t i,uint8_t* c)
{
	//如果没有存入数据，则返回0
  if(FIFOx->Read == FIFOx->Write)	  
  {
    return 0;
  }
  else
  {
		//读取数据，传入到指针c
    *c = FIFOx->Buf[i];	 
		//成功读取数据返回1
    return 1;	
  }
}
