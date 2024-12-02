#include "stm32f10x.h"
#include "led.h"

/*******************************************************************************
* Function Name : USART1_Config
* Description   : LED1开启
* Return        : None 
*******************************************************************************/
void BoardLED1_Config(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_InitTypeDef gpioInit;
	gpioInit.GPIO_Mode=GPIO_Mode_Out_PP;
	gpioInit.GPIO_Pin=GPIO_Pin_1;
	gpioInit.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_Init(GPIOB,&gpioInit);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	gpioInit.GPIO_Mode=GPIO_Mode_Out_PP;
	gpioInit.GPIO_Pin=GPIO_Pin_5;
	gpioInit.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_Init(GPIOA,&gpioInit);

	gpioInit.GPIO_Mode=GPIO_Mode_Out_PP;
	gpioInit.GPIO_Pin=GPIO_Pin_6;
	gpioInit.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_Init(GPIOA,&gpioInit);

	gpioInit.GPIO_Mode=GPIO_Mode_Out_PP;
	gpioInit.GPIO_Pin=GPIO_Pin_7;
	gpioInit.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_Init(GPIOA,&gpioInit);
}

void ShowLedConfig(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	GPIO_InitTypeDef gpioInit;
	gpioInit.GPIO_Mode=GPIO_Mode_Out_PP;
	gpioInit.GPIO_Pin=GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
	gpioInit.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_Init(GPIOA,&gpioInit);
}
void ShowRedLed(void){
	GPIO_ResetBits(GPIOA,GPIO_Pin_5);
	GPIO_SetBits(GPIOA,GPIO_Pin_6);
}
void ShowGreenLed(void){
	GPIO_SetBits(GPIOA,GPIO_Pin_5);
	GPIO_ResetBits(GPIOA,GPIO_Pin_6);
}
void OpenRemoteLed(void){
    GPIO_ResetBits(GPIOA,GPIO_Pin_4);
}	
void CloseRemoteLed(void){
    GPIO_SetBits(GPIOA,GPIO_Pin_4);
}
void OpenValve(void){
	GPIO_ResetBits(GPIOA,GPIO_Pin_7);
}
void CloseValve(void){
	GPIO_SetBits(GPIOA,GPIO_Pin_7);
}
/*******************************************************************************
* Function Name : LED1_On
* Description   : LED1开启
* Return        : None 
*******************************************************************************/
void BoardLED1_On(void){
	GPIO_SetBits(GPIOB,GPIO_Pin_1);
}

/*******************************************************************************
* Function Name : LED1_Off
* Description   : LED1开启
* Return        : None 
*******************************************************************************/
//LED1关闭
void BoardLED1_Off(void){
	GPIO_ResetBits(GPIOB,GPIO_Pin_1);
}
