#include "blade_conf.h"

#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "stdarg.h"
#include "timer.h"
/*
���ļ���ΪCLIģ�����ʹ�ã����ڴ�ͳ�������ж��ǻ���UARTʵ�֣�
���Խӿ�������buf������������UART������ϰ�ߡ�
Ϊ�˷�ֹ��ͬ��֧���뻹��Ҫ�޸ģ���ʱ�Ȳ��޸�UART->CLI������ԭ���ơ�
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
//  * ������: myPrintf
//  * ��������: ��ӡ��ʽ�ַ���
//  * ����: 1. ������ʽ�����ַ�����ַ 2.�ɱ��
//  * ����ֵ: ��
// */
// void SelfCliPrintf(char *format, ...)
// {
//     va_list ap; //��ʼ��ָ��ɱ�����б��ָ��
//     char buf[256]; //�����ſɱ����ת���ɸ�ʽ���ַ��������顣
//     int result; //������������ڵ��ú�����
//     memset(buf,0,256);
//     va_start(ap, format); //����һ���ɱ�����ĵ�ַ����ap����apָ��ɱ�����б�Ŀ�ʼ��
//     result = vsprintf(buf, format, ap); //������ap��format����ת���γɸ�ʽ���ַ�������������ʾ���ַ�����
//     va_end(ap);//������ap��λ��
//     if (result >= 0)
//     {
//     		printf(buf); //��ӡbuf���飨�ַ������飩
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
