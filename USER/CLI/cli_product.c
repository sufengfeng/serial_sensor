#include "blade_conf.h"

#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "stdarg.h"
#include "timer.h"
/*
该文件是为CLI模块服务使用，由于传统的命令行都是基于UART实现，
所以接口命名和buf命名都保留了UART的命名习惯。
为了防止不同分支代码还需要修改，暂时先不修改UART->CLI。保留原名称。
*/


extern uint32_t UART1_rcev_index;
unsigned int Uart1Rptr = 0;
extern char UART1_rcev_buff[UART1_BUFFER_LEN];




char UART_ReceiveChar(void) 
{
	char c;

	if(Uart1Rptr == UART1_rcev_index)
	{
		return 0;
	}
	
	if(Uart1Rptr < sizeof(UART1_rcev_buff))
	{
		c = UART1_rcev_buff[Uart1Rptr++];
		UART1_rcev_buff[Uart1Rptr-1] =0;
	}
	else
	{
		Uart1Rptr = 0;
		c = UART1_rcev_buff[Uart1Rptr++];
		UART1_rcev_buff[Uart1Rptr-1] =0;
	}
	
	return c;
}

char UART_Delay_ReceiveChar(void) 
{
	char c;
	delay(20);

	c = UART_ReceiveChar();	
	return c;
}

// /*
//  * 函数名: myPrintf
//  * 函数功能: 打印格式字符串
//  * 参数: 1. 包含格式符的字符串地址 2.可变参
//  * 返回值: 无
// */
// void SelfCliPrintf(char *format, ...)
// {
//     va_list ap; //初始化指向可变参数列表的指针
//     char buf[256]; //定义存放可变参数转化成格式化字符串的数组。
//     int result; //定义变量，用于调用函数。
//     memset(buf,0,256);
//     va_start(ap, format); //将第一个可变参数的地址赋给ap，即ap指向可变参数列表的开始。
//     result = vsprintf(buf, format, ap); //将参数ap和format进行转化形成格式化字符串，即可以显示的字符串。
//     va_end(ap);//将参数ap复位。
//     if (result >= 0)
//     {
//     		printf(buf); //打印buf数组（字符串数组）
//     }
// }


int do_setloglevel(int argc, char *argv[])
{
	uint8_t loglevel=4;
	if(argc == 2)
	{
		loglevel = atoi(argv[1]);
		SetLogLevel(loglevel);
		printf("SetLogLevel[%d]\n\r",loglevel);
	}else{
		printf("cmd error");
		return -1;
	}
	return 0;
}

int uputc(char ch)
{ 	
#pragma diag_suppress 2748	
	fputc(ch,(FILE *)NULL);
#pragma diag_default 2748
	return 0;
}

extern int do_reboot(int argc, char *argv[]);
extern int do_setloglevel(int argc, char *argv[]);
void plc_cli_install(void){
	Cli_Register("reboot","reboot",do_reboot);
	Cli_Register("setloglevel","set log level",do_setloglevel);
	//Cli_Register("setcli","set cli ",do_setcli);
	//Cli_Register("version","show robot version",do_show_version);
}

#define UART1_BUFFER_LEN 256
uint32_t UART1_rcev_index=0;	/* TODO: rename */
char UART1_rcev_buff[UART1_BUFFER_LEN];

void USART3_IRQHandler_Process(const char *Uart1RxBuffer, uint32_t len)
{
	if(len >=UART1_BUFFER_LEN)
	{
		len = UART1_BUFFER_LEN;
	}

	if((UART1_BUFFER_LEN-UART1_rcev_index)>=len)
	{
		memcpy(&UART1_rcev_buff[UART1_rcev_index],(const char *)Uart1RxBuffer,len);
		UART1_rcev_index = UART1_rcev_index+len;
		if(UART1_rcev_index == UART1_BUFFER_LEN)
		{
			UART1_rcev_index = 0;
		}
	}
	else
	{
		memcpy(&UART1_rcev_buff[UART1_rcev_index],(const char *)Uart1RxBuffer,UART1_BUFFER_LEN-UART1_rcev_index);
		memcpy(&UART1_rcev_buff[0],
				(const char *)Uart1RxBuffer+(UART1_BUFFER_LEN-UART1_rcev_index),
				len-(UART1_BUFFER_LEN-UART1_rcev_index));
		UART1_rcev_index = len-(UART1_BUFFER_LEN-UART1_rcev_index);
	}
}
