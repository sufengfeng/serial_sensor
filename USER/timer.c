#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "fifo.h"
#include "timer.h"
#include "uart.h"

/*******************************************************************************
* Function Name : TIM2_Config 
* Description   : 初始化TIM2
* Input         : None
* Return        : None 
*******************************************************************************/
void TIM2_Config(void)	//定义1ms计数器
{
	//定义结构体
	TIM_TypeDef* TIMx=TIM2;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	//设置在下一个更新事件装入活动的自动重装载寄存器周期的值,计数到100为10ms
	TIM_TimeBaseStructure.TIM_Period = 2; 
	//预分频系数为36000-1，这样计数器时钟为72MHz/36000 = 2kHz
	TIM_TimeBaseStructure.TIM_Prescaler =36000 - 1;
	//设置时钟分割
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	//TIM向上计数模式
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	//根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
	TIM_TimeBaseInit(TIMx, &TIM_TimeBaseStructure); 
	//使能或者失能指定的TIM中断
	TIM_ITConfig(TIMx,TIM_IT_Update,ENABLE);
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	//使能TIMx外设 定时器使能
	TIM_Cmd(TIMx, ENABLE);		 
}

/*******************************************************************************
* Function Name : TIM2_IRQHandler 
* Description   : TIM2中断服务程序
* Input         : None
* Return        : None 
*******************************************************************************/
void TIM2_IRQHandler(void)
{ 
	//定义结构体
	TIM_TypeDef* TIMx=TIM2;
	//检查指定的TIM中断发生与否:TIM 中断源 
	if (TIM_GetITStatus(TIMx, TIM_IT_Update) != RESET) 
	{
		// Frame_Handler(&FIFO1,USART2);
		// Frame_Handler(&FIFO2,USART1);
		UpdataSoftTimer();
		//清除TIMx的中断待处理位:TIM 中断源 
		TIM_ClearITPendingBit(TIMx, TIM_IT_Update);
		
	};
}


/*******************************************************************************
* Function Name : Frame_Handler 
* Description   : 处理数据帧（提取完整数据帧，并校正数据）
* Input         : None
* Return        : None 
*******************************************************************************/
// void Frame_Handler_(FIFOTypeDef* FIFOx,USART_TypeDef* USARTx){
// 	uint8_t byte;
// 	uint8_t dataLength;
// 	uint8_t frameLength;
// 	uint8_t crc;
// 	//读取判断包头是否正确
// 	if(FIFO_Get(FIFOx,0,&byte)){
// 		if(byte!=0x02){
// 			FIFO_Pop(FIFOx,&byte);
// 		}
// 		else{
// 			//读取机械码并判断是否正确
// 			if(FIFO_Get(FIFOx,1,&byte)){
// 				if(byte!=0x01){
// 					FIFO_Pop(FIFOx,&byte);
// 				}
// 				else{
// 					//读取长度并判断是否正确
// 					if(FIFO_Get(FIFOx,2,&byte)){
// 						if(byte<=0x30){
// 							FIFO_Pop(FIFOx,&byte);
// 						}
// 						else{
// 							//将长度转换为字节长度，ASCII码的数字从30开始
// 							dataLength=byte-0x30;
// 							//数据帧长度=包头+机械码+长度+数据+校验码
// 							frameLength=dataLength+4;
// 							if(FIFO_Get(FIFOx,frameLength-1,&byte)){
// 								Frame_GetCrc(FIFOx,frameLength-1,&crc);
// 								if(byte!=crc){
// 									FIFO_Pop(FIFOx,&byte);
// 								}
// 								else{
// 									USART_SendByte(USARTx,crc);
// 								}
// 							}
// 						}
// 					}
// 				}
// 			}
// 		}
// 	}
// }
