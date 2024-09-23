#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "led.h"
#include "uart.h"
#include "fifo.h"
#include "stdio.h"
volatile uint8_t UART1_RxBuffer[128]={0x00};
volatile uint8_t UART1_RxCount=0;
volatile uint8_t UART1_ReceiveState=0;

volatile uint8_t UART2_RxBuffer[128]={0x00};
volatile uint8_t UART2_RxCount=0;
volatile uint8_t UART2_ReceiveState=0;

volatile uint8_t UART3_RxBuffer[128]={0x00};
volatile uint8_t UART3_RxCount=0;
volatile uint8_t UART3_ReceiveState=0;

volatile uint8_t UART_IO_RxBuffer[128]={0x00};
volatile uint8_t UART_IO_RxCount=0;
volatile uint8_t UART_IO_ReceiveState=0;
/*******************************************************************************
* Function Name  : RCC_Config
* Description    : 外设配置
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void RCC_Config(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA
                       | RCC_APB2Periph_GPIOB
                       | RCC_APB2Periph_AFIO
                       | RCC_APB2Periph_USART1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2|RCC_APB1Periph_TIM2,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
}

/*******************************************************************************
* Function Name : NVIC_Configuration 
* Description   : 中断配置
* Input         : None 
* Return        : None 
*******************************************************************************/
void NVIC_Config(void)
{
	//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	
}

/*******************************************************************************
* Function Name : Frame_Handler 
* Description   : 获取数据帧校验码
* Input         : buf：数组;len：数组长度
* Return        : None 
*******************************************************************************/
uint8_t Frame_CheckSum(volatile uint8_t *buffer, uint8_t length)
{ 
    uint8_t i, res = 0;
    for(i=0; i<length; i++)
    {
        res += buffer[i];
    }
    res = ~res+1;
    return res;
}

/*******************************************************************************
* Function Name : Frame_Handler 
* Description   : 处理数据帧（提取完整数据帧，并校正数据）
* Input         : None
* Return        : None 
*******************************************************************************/
void Frame_Handler_(USART_TypeDef* USARTin,USART_TypeDef* USARTout,volatile uint8_t buffer[],volatile uint8_t count){
	uint8_t frameLength=21;
	//判断是否是取数返回的数据包([0]包头，[1]M(0x4D),ASCII编码，[2~6]水平偏差，[7~11]垂直偏差，[12~15]光强，[16~19]灯高,[20]校验码)
	if(frameLength==count && buffer[0]==0x02 && buffer[1]==0x4D){
		//计算校验码
		uint8_t crc=Frame_CheckSum(buffer,frameLength-1);
		//验证校验码
		if(crc==buffer[frameLength-1]){
			//随机产生一个负100-200的数值替换。例如设置为：-[0x2D]1[0x31]5[0x35]0[0x30]空格[0x20]
			buffer[8]=0x2D;
			buffer[8]=0x31;
			buffer[9]=0x35;
			buffer[10]=0x30;
			buffer[11]=0x20;
			//光强如果小于180且大于0，随机产生一个180-380的数值替换。例如设置为：0[0x30]2[0x32]0[0x30]0[0x30]
			buffer[12]=0x30;
			buffer[13]=0x32;
			buffer[14]=0x30;
			buffer[15]=0x30;
			//重新设置校验码
			buffer[frameLength-1]=Frame_CheckSum(buffer,frameLength-1);
		}
	}
	//发送数据
	uint8_t index=0;
	while(count--)
	{
		USART_SendData(USARTout, buffer[index++]);	
		while(USART_GetFlagStatus(USARTout, USART_FLAG_TC)== RESET);
	}
}
uint8_t PR4_COMMAND_PRESSURE[]="PR";
uint8_t PACE_COMMAND_PRESSURE[]={0x3A,0x53,0x65,0x6E,0x73,0x3F,0x0D,0x0A};
uint8_t PACE_RESULT_HEAD[]=":SEHS:PRES";
#define RP_COMMAND	"*RP?:25\r\n"
float g_nV_psi=0;
/*******************************************************************************
* Function Name : UART2_Frame_Handler 
* Description   : 处理数据帧（提取完整数据帧，并校正数据）
* Input         : None
* Return        : None 
*******************************************************************************/
void UART2_Frame_Handler(USART_TypeDef* USARTtype,volatile uint8_t buffer[],volatile uint8_t count){
	printf("[%s%d][%s][%d]\n",__func__,__LINE__,buffer,count);
	if(strcmp("!rp=",(const char*)buffer)<=0){	//收到命令	"!rp=-0.337:74\r\n"
		sscanf((const char*)buffer,"!rp=%f",&g_nV_psi);
		printf("[%s%d][%f]\n",__func__,__LINE__,g_nV_psi);
	}else{
		;
	}
}
#define PR_COMMAND_CLS	"*CLS"	//清屏
#define PR_COMMAND_LOCAL	"LOCAL"	//设置为本地模式
#define PR_COMMAND_REMOTE	"REMOTE"	//设置为远程模式
#define PR_COMMAND_AUTOZERO	"AUTOZERO"	//自动校零UTOZERO
#define PR_COMMAND_RATE	"RATE"	//查询采样率
#define PR_COMMAND_PR	"PR"	//查询压力值
#define PR_RESPONE_OK "OK\n\r"	//响应OK

//获取字符串校验和	
uint8_t Frame_CheckSum_(uint8_t *data,uint8_t len){
	uint8_t sum=0;
	while(len--){
		sum+=*data++;
	}
	return sum;
}
//滑动窗口滤波	
float Frame_SlidingWindowFilter(float *data,uint8_t len){
	float sum=0;
	for(uint8_t i=0;i<len;i++){
		sum+=data[i];
	}
}
/*******************************************************************************
* Function Name : UART_IO_Frame_Handler 
* Description   : 处理数据帧（提取完整数据帧，并校正数据）
* Input         : None
* Return        : None 
*******************************************************************************/
void UART_IO_Frame_Handler(USART_TypeDef* USARTtype,volatile uint8_t buffer[],volatile uint8_t count){
	printf("[%s%d][%s][%d]\n",__func__,__LINE__,buffer,count);
	if(strcmp((const char*)buffer,PR_COMMAND_CLS)<=0){	//收到命令	
		Uart_SendByteStr(PR_RESPONE_OK,strlen(PR_RESPONE_OK));
	}else  if(strcmp((const char*)buffer,PR_COMMAND_LOCAL)<=0){
		Uart_SendByteStr(PR_COMMAND_LOCAL,strlen(PR_COMMAND_LOCAL));
	}
	else if(strcmp((const char*)buffer,PR_COMMAND_REMOTE)<=0){
		Uart_SendByteStr(PR_COMMAND_REMOTE,strlen(PR_COMMAND_REMOTE));
	}else if(strcmp((const char*)buffer,PR_COMMAND_AUTOZERO)<=0){
		Uart_SendByteStr(PR_COMMAND_AUTOZERO,strlen(PR_COMMAND_AUTOZERO));
	}else if(strcmp((const char*)buffer,PR_COMMAND_RATE)<=0){
		Uart_SendByteStr(PR_COMMAND_RATE,strlen(PR_COMMAND_RATE));
	}else if(strcmp((const char*)buffer,PR_COMMAND_PR)<=0){
		Uart_SendByteStr(PR_COMMAND_PR,strlen(PR_COMMAND_PR));
	}else{
		;
	}
}
/*******************************************************************************
* Function Name : Frame_Handler 
* Description   : 处理数据帧（提取完整数据帧，并校正数据）
* Input         : None
* Return        : None 
*******************************************************************************/
void Frame_Handler(USART_TypeDef* USARTtype,volatile uint8_t buffer[],volatile uint8_t count){
	if(USARTtype==USART1){
		if(strcmp(PR4_COMMAND_PRESSURE,(const char*)buffer)<=0){	//收到命令
			for(int i=0;i<8;i++){	
				while(USART_GetFlagStatus(USART2, USART_FLAG_TC) != SET);
    			USART2->DR = (u8) (PACE_COMMAND_PRESSURE[i]);
			}
			// printf("[%s%d][%s][%d]\n",__func__,__LINE__,PACE_COMMAND_PRESSURE,count);
		}else{
			;
		}
	}else{			//如果串口2收到数据，则通过串口1回传上报
		if(strcmp(PACE_RESULT_HEAD,(const char*)buffer)<=0){
			// printf("[%s%d]\n",__func__,__LINE__);//:SENS:PRES 14.9781159
			char tmp_str[128];
			float V_mpa,V_psi=0;
			sscanf((const char*)buffer,"%s %f",tmp_str,&V_psi);
			
			V_mpa=V_psi*0.006895;
			//V_mpa=V_psi*1;
			printf("%.8f\r\n",V_mpa);
			memset(tmp_str,0,128);
			memset((void*)buffer,0,128);		//传输完成，清除数据
		}
	}
}
//1ms回调事件
void Func_Task_1ms01(void){
	
}
//10ms回调事件
void Func_Task_10ms01(void){
	
}
//100ms回调事件
void Func_Task_100ms01(void){
	// static uint8_t counter=0;
	// if(counter++>10){
	// 	counter=0;

	// }
}


//100ms回调事件
void Func_Task_1000ms01(void){
	static uint8_t flag=0;
	if(flag){
	// 		char str[30];
	// float a = 123.123456, b = 90.12;

	// sprintf(str, "获取的小数为：%.2f\n", a); // 此处，只是把双引号中的字符串赋值给str数组，并没打印到屏幕
	// printf("%s\n", str);
	// sprintf(str, "%.2f----%6.2f\n", a, b);	// 把2个小数拼接到一个字符串中
	// printf("%s\n", str);
		flag=0;
		LED1_On();
		// printf("running...\n");	
		// Uart_SendByteStr(PR_RESPONE_OK,strlen(PR_RESPONE_OK));
		USART2_SendStr(RP_COMMAND,strlen(RP_COMMAND));

		char sendBuffer[128];
		sprintf(sendBuffer,"home_page0.t0.txt=\"%f\"\xff\xff\xff",g_nV_psi);
		USART1_SendStr(sendBuffer,strlen(sendBuffer));
		
		sprintf(sendBuffer,"*RP?:");
		int ret=Frame_CheckSum_(sendBuffer,strlen(sendBuffer));
		printf("running...[%d]\n",ret);	
		ret=Frame_CheckSum(sendBuffer,strlen(sendBuffer));
		printf("running...[%d]\n",ret);	
	}else{
		flag=1;
		LED1_Off();
	}
}
//1ms中断事件
void Func_Task_Interrupt(void){

}
/*****************************callback end***************************************************/
//多任务软定时器		异步任务
volatile SoftTimer g_sTimerArray[]={
	{0,1,Func_Task_1ms01},
	{5,9,Func_Task_10ms01},
	{37,99,Func_Task_100ms01},
	{373,999,Func_Task_1000ms01},
};

//Systik定时器中断更新定时器 10ms调度一次
void UpdataSoftTimer(void ){
	Func_Task_Interrupt();		//中断回调服务函数
	for(uint32_t i=0;i<sizeof(g_sTimerArray)/sizeof(SoftTimer);i++){			//更新软定时器
		g_sTimerArray[i].m_nCounter++;
	}
}

//定时器任务调度
void TaskSchedule(void){
	for(uint32_t i=0;i<sizeof(g_sTimerArray)/sizeof(SoftTimer);i++){			//更新软定时器
			if(g_sTimerArray[i].m_nCounter>g_sTimerArray[i].m_nMaxCounter){
				g_sTimerArray[i].m_nCounter=0;
				(*(g_sTimerArray[i].funcCallBack))();
		}
	}
}
extern int main1(void);
int main(void){
	uint8_t byte;
	RCC_Config();
	NVIC_Config();
	LED1_Config();
	LED1_On();
	USART1_Config();
	USART2_Config();
	USART3_Config();
	TIM2_Config();
	
	USART1_SendByte(0x01);
	// USART2_SendByte(0x02);	
	USART2_SendStr(RP_COMMAND,strlen(RP_COMMAND));

	USART3_SendByte(0x01);		//第一个字节发送异常
	//USART3_SendByte(0x02);
	//USART3_SendByte(0x03);
	USART_GPIO_Init();		//初始化串口GPIO
	Timer3_Init(9600,8);			//初始化定时器3
	Uart_SendByte(0x04);		
	printf("Init Done\n");
	// main1();
	while(1){
		//如果UART1接收到1帧数据
		if(UART1_ReceiveState==1)	//串口屏
		{
			UART1_ReceiveState=0;
			Frame_Handler(USART1,UART1_RxBuffer,UART1_RxCount);
			UART1_RxCount=0;
		}
		//如果UART2接收到1帧数据
		if(UART2_ReceiveState==1)	//传感器
		{
			UART2_ReceiveState=0;
			UART2_Frame_Handler(USART2,UART2_RxBuffer,UART2_RxCount);
			UART2_RxCount=0;
		}
		//如果UART2接收到1帧数据
		if(UART_IO_ReceiveState==1)	//外部IO
		{
			UART_IO_Frame_Handler(NULL,UART_IO_RxBuffer,UART_IO_RxCount);
			UART_IO_RxCount=0;
			UART_IO_ReceiveState=0;
		}
		TaskSchedule();
		// if(GetUartIOCounter()>0)	{
		// 	printf("GetUartIOCounter:%d\n",GetUartIOCounter());
		// 	printf("Received data: %d\n",Uart_ReceiveByte());
		// }
	}
}
