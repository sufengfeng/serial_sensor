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
/*******************************************************************************
* Function Name  : RCC_Config
* Description    : ��������
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
}

/*******************************************************************************
* Function Name : NVIC_Configuration 
* Description   : �ж�����
* Input         : None 
* Return        : None 
*******************************************************************************/
void NVIC_Config(void)
{
	//����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	
}

/*******************************************************************************
* Function Name : Frame_Handler 
* Description   : ��ȡ����֡У����
* Input         : buf������;len�����鳤��
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
* Description   : ��������֡����ȡ��������֡����У�����ݣ�
* Input         : None
* Return        : None 
*******************************************************************************/
void Frame_Handler_(USART_TypeDef* USARTin,USART_TypeDef* USARTout,volatile uint8_t buffer[],volatile uint8_t count){
	uint8_t frameLength=21;
	//�ж��Ƿ���ȡ�����ص����ݰ�([0]��ͷ��[1]M(0x4D),ASCII���룬[2~6]ˮƽƫ�[7~11]��ֱƫ�[12~15]��ǿ��[16~19]�Ƹ�,[20]У����)
	if(frameLength==count && buffer[0]==0x02 && buffer[1]==0x4D){
		//����У����
		uint8_t crc=Frame_CheckSum(buffer,frameLength-1);
		//��֤У����
		if(crc==buffer[frameLength-1]){
			//�������һ����100-200����ֵ�滻����������Ϊ��-[0x2D]1[0x31]5[0x35]0[0x30]�ո�[0x20]
			buffer[8]=0x2D;
			buffer[8]=0x31;
			buffer[9]=0x35;
			buffer[10]=0x30;
			buffer[11]=0x20;
			//��ǿ���С��180�Ҵ���0���������һ��180-380����ֵ�滻����������Ϊ��0[0x30]2[0x32]0[0x30]0[0x30]
			buffer[12]=0x30;
			buffer[13]=0x32;
			buffer[14]=0x30;
			buffer[15]=0x30;
			//��������У����
			buffer[frameLength-1]=Frame_CheckSum(buffer,frameLength-1);
		}
	}
	//��������
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
/*******************************************************************************
* Function Name : Frame_Handler 
* Description   : ��������֡����ȡ��������֡����У�����ݣ�
* Input         : None
* Return        : None 
*******************************************************************************/
void Frame_Handler(USART_TypeDef* USARTtype,volatile uint8_t buffer[],volatile uint8_t count){
	if(USARTtype==USART1){
		if(strcmp(PR4_COMMAND_PRESSURE,(const char*)buffer)<=0){	//�յ�����
			for(int i=0;i<8;i++){	
				while(USART_GetFlagStatus(USART2, USART_FLAG_TC) != SET);
    			USART2->DR = (u8) (PACE_COMMAND_PRESSURE[i]);
			}
			// printf("[%s%d][%s][%d]\n",__func__,__LINE__,PACE_COMMAND_PRESSURE,count);
		}else{
			;
		}
	}else{			//�������2�յ����ݣ���ͨ������1�ش��ϱ�
		if(strcmp(PACE_RESULT_HEAD,(const char*)buffer)<=0){
			// printf("[%s%d]\n",__func__,__LINE__);//:SENS:PRES 14.9781159
			char tmp_str[128];
			float V_mpa,V_psi=0;
			sscanf((const char*)buffer,"%s %f",tmp_str,&V_psi);
			
			V_mpa=V_psi*0.006895;
			//V_mpa=V_psi*1;
			printf("%.8f\r\n",V_mpa);
			memset(tmp_str,0,128);
			memset((void*)buffer,0,128);		//������ɣ��������
		}
	}
}
//1ms�ص��¼�
void Func_Task_1ms01(void){
	
}
//10ms�ص��¼�
void Func_Task_10ms01(void){
	
}
//100ms�ص��¼�
void Func_Task_100ms01(void){
	// static uint8_t counter=0;
	// if(counter++>10){
	// 	counter=0;

	// }
}
//100ms�ص��¼�
void Func_Task_1000ms01(void){
	static uint8_t flag=0;
	if(flag){
	// 		char str[30];
	// float a = 123.123456, b = 90.12;

	// sprintf(str, "��ȡ��С��Ϊ��%.2f\n", a); // �˴���ֻ�ǰ�˫�����е��ַ�����ֵ��str���飬��û��ӡ����Ļ
	// printf("%s\n", str);
	// sprintf(str, "%.2f----%6.2f\n", a, b);	// ��2��С��ƴ�ӵ�һ���ַ�����
	// printf("%s\n", str);
		flag=0;
		LED1_On();
		printf("running...\n");
	}else{
		flag=1;
		LED1_Off();
	}
}
//1ms�ж��¼�
void Func_Task_Interrupt(void){

}
/*****************************callback end***************************************************/
//��������ʱ��		�첽����
volatile SoftTimer g_sTimerArray[]={
	{0,1,Func_Task_1ms01},
	{5,9,Func_Task_10ms01},
	{37,99,Func_Task_100ms01},
	{373,999,Func_Task_1000ms01},
};

//Systik��ʱ���жϸ��¶�ʱ�� 10ms����һ��
void UpdataSoftTimer(void ){
	Func_Task_Interrupt();		//�жϻص�������
	for(uint32_t i=0;i<sizeof(g_sTimerArray)/sizeof(SoftTimer);i++){			//������ʱ��
		g_sTimerArray[i].m_nCounter++;
	}
}

//��ʱ���������
void TaskSchedule(void){
	for(uint32_t i=0;i<sizeof(g_sTimerArray)/sizeof(SoftTimer);i++){			//������ʱ��
			if(g_sTimerArray[i].m_nCounter>g_sTimerArray[i].m_nMaxCounter){
				g_sTimerArray[i].m_nCounter=0;
				(*(g_sTimerArray[i].funcCallBack))();
		}
	}
}
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
	USART2_SendByte(0x02);
	
	USART3_SendByte(0x01);		//��һ���ֽڷ����쳣
	//USART3_SendByte(0x02);
	//USART3_SendByte(0x03);
	printf("Init Done\n");
	while(1){
		//���UART1���յ�1֡����
		if(UART1_ReceiveState==1)
		{
			UART1_ReceiveState=0;
			Frame_Handler(USART1,UART1_RxBuffer,UART1_RxCount);
			UART1_RxCount=0;
		}
		//���UART2���յ�1֡����
		if(UART2_ReceiveState==1)
		{
			UART2_ReceiveState=0;
			Frame_Handler(USART2,UART2_RxBuffer,UART2_RxCount);
			UART2_RxCount=0;
		}
		TaskSchedule();
	}
	// while(1){
	// 	//�����ȡָ�벻����д��ָ�룬˵��������������
	// 	if(FIFO2.Write != FIFO2.Read) 
	// 	{
	// 		//�����ȡ���ݳɹ�������1
	// 		if(FIFO2_GetChar(&byte))		
	// 		{
	// 			//�����յ������ݷ��ͣ����أ���USART3
	// 			USART2_SendByte(byte);	
	// 		}
	// 	}
	// 	//�����ȡָ�벻����д��ָ�룬˵��������������
	// 	if(FIFO3.Write != FIFO3.Read) 
	// 	{
	// 		//�����ȡ���ݳɹ�������1
	// 		if(FIFO3_GetChar(&byte))
	// 		{
	// 			//�����յ������ݷ��ͣ����أ���USART3
	// 			USART3_SendByte(byte);	
	// 		}
	// 	}
	// 	TaskSchedule();
	// }
}
