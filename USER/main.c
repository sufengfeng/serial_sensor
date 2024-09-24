#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "led.h"
#include "uart.h"
#include "fifo.h"
#include "stdio.h"
#include "timer.h"
volatile uint8_t UART1_RxBuffer[128] = {0x00};
volatile uint8_t UART1_RxCount = 0;
volatile uint8_t UART1_ReceiveState = 0;

volatile uint8_t UART2_RxBuffer[128] = {0x00};
volatile uint8_t UART2_RxCount = 0;
volatile uint8_t UART2_ReceiveState = 0;

volatile uint8_t UART3_RxBuffer[128] = {0x00};
volatile uint8_t UART3_RxCount = 0;
volatile uint8_t UART3_ReceiveState = 0;

volatile uint8_t UART_IO_RxBuffer[128] = {0x00};
volatile uint8_t UART_IO_RxCount = 0;
volatile uint8_t UART_IO_ReceiveState = 0;
/*******************************************************************************
 * Function Name  : RCC_Config
 * Description    : ��������
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void RCC_Config(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 | RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
}

/*******************************************************************************
 * Function Name : NVIC_Configuration
 * Description   : �ж�����
 * Input         : None
 * Return        : None
 *******************************************************************************/
void NVIC_Config(void)
{
	// ����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�
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
	for (i = 0; i < length; i++)
	{
		res += buffer[i];
	}
	res = ~res + 1;
	return res;
}

/*******************************************************************************
 * Function Name : Frame_Handler
 * Description   : ��������֡����ȡ��������֡����У�����ݣ�
 * Input         : None
 * Return        : None
 *******************************************************************************/
void Frame_Handler_(USART_TypeDef *USARTin, USART_TypeDef *USARTout, volatile uint8_t buffer[], volatile uint8_t count)
{
	uint8_t frameLength = 21;
	// �ж��Ƿ���ȡ�����ص����ݰ�([0]��ͷ��[1]M(0x4D),ASCII���룬[2~6]ˮƽƫ�[7~11]��ֱƫ�[12~15]��ǿ��[16~19]�Ƹ�,[20]У����)
	if (frameLength == count && buffer[0] == 0x02 && buffer[1] == 0x4D)
	{
		// ����У����
		uint8_t crc = Frame_CheckSum(buffer, frameLength - 1);
		// ��֤У����
		if (crc == buffer[frameLength - 1])
		{
			// �������һ����100-200����ֵ�滻����������Ϊ��-[0x2D]1[0x31]5[0x35]0[0x30]�ո�[0x20]
			buffer[8] = 0x2D;
			buffer[8] = 0x31;
			buffer[9] = 0x35;
			buffer[10] = 0x30;
			buffer[11] = 0x20;
			// ��ǿ���С��180�Ҵ���0���������һ��180-380����ֵ�滻����������Ϊ��0[0x30]2[0x32]0[0x30]0[0x30]
			buffer[12] = 0x30;
			buffer[13] = 0x32;
			buffer[14] = 0x30;
			buffer[15] = 0x30;
			// ��������У����
			buffer[frameLength - 1] = Frame_CheckSum(buffer, frameLength - 1);
		}
	}
	// ��������
	uint8_t index = 0;
	while (count--)
	{
		USART_SendData(USARTout, buffer[index++]);
		while (USART_GetFlagStatus(USARTout, USART_FLAG_TC) == RESET)
			;
	}
}
#define WINDOW_SIZE 5

float sliding_window_filter(float current_value)
{
	static float history_data[WINDOW_SIZE];
	static int index = 0;
	static int count = 0;

	history_data[index++] = current_value;
	if (index >= WINDOW_SIZE)
	{
		index = 0;
	}
	if (count < WINDOW_SIZE)
	{
		count++;
	}

	float sum = 0;
	for (int i = 0; i < count; i++)
	{
		sum += history_data[i];
	}
	return sum / count;
}

uint8_t PR4_COMMAND_PRESSURE[] = "PR";
uint8_t PACE_COMMAND_PRESSURE[] = {0x3A, 0x53, 0x65, 0x6E, 0x73, 0x3F, 0x0D, 0x0A};
uint8_t PACE_RESULT_HEAD[] = ":SEHS:PRES";

#define SENSOR_COMMAND_RP "*RP?:25\r\n" // ��ѯѹ��ֵ
#define SENSOR_COMMAND_AZ "*IZ:63\r\n"	// �Զ�У��
// [Func_Task_1000ms01303][*RP?:][25]
// [Func_Task_1000ms01306][*IZ:][63]
float g_fV_mbar = 0;
float g_fV_psi = 0;	 // psiѹ��ֵ
float g_fV_rate = 0; // psiѹ��ֵ�仯��

/*******************************************************************************
 * Function Name : UART2_Frame_Handler
 * Description   : ��������֡����ȡ��������֡����У�����ݣ�
 * Input         : None
 * Return        : None
 *******************************************************************************/
void UART2_Frame_Handler(USART_TypeDef *USARTtype, volatile uint8_t buffer[], volatile uint8_t count)
{
	// printf("[%s%d][%s][%d]\n", __func__, __LINE__, buffer, count);
	if (strncmp((const char *)buffer, "!rp=", strlen("!rp=")) <= 0) // �յ�����	"!rp=-0.337:74\r\n"
	{
		float tmpValue = 0;
		sscanf((const char *)buffer, "!rp=%f", &tmpValue);
		g_fV_mbar = sliding_window_filter(tmpValue);
		// printf("[%s%d][%f]\n", __func__, __LINE__, g_fV_mbar);
	}
}

#define PR_COMMAND_CLS "*CLS\r\n"			   // ����
#define PR_COMMAND_LOCAL "LOCAL\r\n"		   // ����Ϊ����ģʽ
#define PR_COMMAND_REMOTE "REMOTE\r\n"		   // ����ΪԶ��ģʽ
#define PR_COMMAND_COM_PARAMETER "COM1 "	   // ���ô��ڲ���
#define PR_COMMAND_AUTOZERO "AUTOZERO=RUN\r\n" // �Զ�У��UTOZERO
#define PR_COMMAND_RATE "RATE\r\n"			   // ��ѯ������
#define PR_COMMAND_PR "PR\r\n"				   // ��ѯѹ��ֵ

#define PR_RESPONE_OK "OK\r\n" // ��ӦOK

// ��ȡ�ַ���У���
uint8_t Frame_CheckSum_(uint8_t *data, uint8_t len)
{
	uint16_t sum = 0;
	while (len--)
	{
		sum += *data++;
	}
	return sum % 100;
}

/*******************************************************************************
 * Function Name : UART_IO_Frame_Handler
 * Description   : ��������֡����ȡ��������֡����У�����ݣ�
 * Input         : None
 * Return        : None
 *******************************************************************************/
void UART_IO_Frame_Handler(USART_TypeDef *USARTtype, volatile uint8_t buffer[], volatile uint8_t len)
{
	printf("[%s%d][%s][%d]\n", __func__, __LINE__, buffer, len);
	if (!strncmp((const char *)buffer, PR_COMMAND_CLS, strlen(PR_COMMAND_CLS))) // ����
	{																			// �յ�����
		Uart_SendByteStr(PR_RESPONE_OK, strlen(PR_RESPONE_OK));
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_LOCAL, strlen(PR_COMMAND_LOCAL))) // ����Ϊ����ģʽ
	{
		Uart_SendByteStr(buffer, len);
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_REMOTE, strlen(PR_COMMAND_REMOTE))) // ����ΪԶ��ģʽ
	{
		Uart_SendByteStr(buffer, len);
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_AUTOZERO, strlen(PR_COMMAND_AUTOZERO))) // �Զ�У��
	{
		SendComandAutoZero();
		Uart_SendByteStr(PR_RESPONE_OK, strlen(PR_RESPONE_OK));
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_RATE, strlen(PR_COMMAND_RATE))) // ��ѯ������
	{
		char sendBuffer[128];
		sprintf(sendBuffer, "%f psi/s\r\n", g_fV_rate);
		Uart_SendByteStr(buffer, len);
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_PR, strlen(PR_COMMAND_PR)))
	{
		char sendBuffer[128];
		if (abs(g_fV_psi) > 0.00145)
		{
			sprintf(sendBuffer, "NR      %.3f psi g\r\n", g_fV_psi);
			Uart_SendByteStr(sendBuffer, strlen(sendBuffer));
		}
		else
		{
			sprintf(sendBuffer, "R       %.3f psi g\r\n", g_fV_psi);
			Uart_SendByteStr(sendBuffer, strlen(sendBuffer));
		}
		Uart_SendByteStr(buffer, len);
	}
	else
	{
		;
	}
}
#define UI_COMMAND_AZ "AutoZ" // �Զ�У��
/*******************************************************************************
 * Function Name : UART1_Frame_Handler
 * Description   : ��������֡����ȡ��������֡����У�����ݣ�
 * Input         : None
 * Return        : None
 *******************************************************************************/
void UART1_Frame_Handler(USART_TypeDef *USARTtype, volatile uint8_t buffer[], volatile uint8_t len)
{
	printf("[%s%d][%s][%d]\n", __func__, __LINE__, buffer, len);
	if (!strncmp((const char *)buffer,UI_COMMAND_AZ,strlen(UI_COMMAND_AZ)))	// �յ�����	
	{ 
		SendComandAutoZero();
		printf("[%s][%d]SendComandAutoZero\n",	__func__, __LINE__);
	}	else if(!strncmp((const char *)buffer, "BaudRate=", strlen("BaudRate=")))	//BaudRate=9600;WordLength=8;StopBits=1;Parity=None
	{
		GlobalBasicParam*p_sGlobalBasicParam=(void *)GetBasicParamHandle();
		char tmpBuffer[128];
		sscanf((const char *)buffer, "BaudRate=%d;WordLength=%d;StopBits=%d;Parity=%s", &(p_sGlobalBasicParam->m_nBaudRate),&(p_sGlobalBasicParam->m_nWordLength),&(p_sGlobalBasicParam->m_nStopBits),tmpBuffer);
		if (!strncmp((const char *)buffer,"None",strlen("None")))
		{
			p_sGlobalBasicParam->m_nParity=enumNone;
		}else if (	!strncmp((const char *)buffer, "Odd", strlen("Odd")))
		{
			p_sGlobalBasicParam->m_nParity=enumOdd;
		}else{	
			p_sGlobalBasicParam->m_nParity=enumEven;
		}
		PrintBasicParam(p_sGlobalBasicParam);
	}	else if(!strncmp((const char *)buffer, "SaveSerialParam", strlen("SaveSerialParam")))	//"SaveSerialParam"	���洮�ڲ���		
	{
	    printf("[%s][%d]SaveSerialParam\n",	__func__, __LINE__);
		SaveCurrentBasicParam();
		Reboot();
	}else if(!strncmp((const char *)buffer, "page_setting", strlen("page_setting")))	//"SaveSerialParam"	���洮�ڲ���		
	{
	    UpdateUiInit();	//
	}
}
// 1ms�ص��¼�
void Func_Task_1ms01(void)
{
}
// 10ms�ص��¼�
void Func_Task_10ms01(void)
{
}
// 100ms�ص��¼�
void Func_Task_100ms01(void)
{
	USART2_SendStr(SENSOR_COMMAND_RP, strlen(SENSOR_COMMAND_RP)); // ���Ͳ�ѯ����
}
int SendComandAutoZero(void)
{
	return USART2_SendStr(SENSOR_COMMAND_AZ, strlen(SENSOR_COMMAND_AZ)); // ���Ͳ�ѯ����
}
// ���ڸ������ݵ�������
int UpdateUiPeriod(void)
{
	static float last_V_psi = 0;
	last_V_psi = g_fV_psi;
	char sendBuffer[128];
	g_fV_psi = 0.0145038 * g_fV_mbar;
	sprintf(sendBuffer, "home_page0.t0.txt=\"%f\"\xff\xff\xff", g_fV_psi);
	USART1_SendStr(sendBuffer, strlen(sendBuffer));

	g_fV_rate = g_fV_psi - last_V_psi;
	sprintf(sendBuffer, "home_page0.t3.txt=\"%f\"\xff\xff\xff", g_fV_rate);
	USART1_SendStr(sendBuffer, strlen(sendBuffer));
}
// ���ڸ������ݵ�������
int UpdateUiInit(void)
{
	char sendBuffer[128];
	char tmpBuffer[128];
	GlobalBasicParam*p_sGlobalBasicParam=(void *)GetBasicParamHandle();
	sprintf(sendBuffer, "setting.cb0.txt=\"%d\"\xff\xff\xff", p_sGlobalBasicParam->m_nBaudRate);	// ������
	USART1_SendStr(sendBuffer, strlen(sendBuffer));

	sprintf(sendBuffer, "setting.cb1.txt=\"%d\"\xff\xff\xff", p_sGlobalBasicParam->m_nWordLength);	// ����λ
	USART1_SendStr(sendBuffer, strlen(sendBuffer));

	sprintf(sendBuffer, "setting.cb2.txt=\"%d\"\xff\xff\xff", p_sGlobalBasicParam->m_nStopBits);	// ֹͣλ
	USART1_SendStr(sendBuffer, strlen(sendBuffer));
	
	if(p_sGlobalBasicParam->m_nParity ==enumNone){
		sprintf(tmpBuffer, "None");
	}else if (	p_sGlobalBasicParam->m_nParity==enumOdd) 
	{
		sprintf(tmpBuffer, "Odd");
	}else
	{
		sprintf(tmpBuffer, "Even");
	}
	
	sprintf(sendBuffer, "setting.cb3.txt=\"%s\"\xff\xff\xff", tmpBuffer);	// ��żУ��
	USART1_SendStr(sendBuffer, strlen(sendBuffer));
}
// 100ms�ص��¼�
void Func_Task_1000ms01(void)
{
	static uint8_t flag = 0;
	if (flag)
	{
		flag = 0;
		LED1_On();
	}
	else
	{
		flag = 1;
		LED1_Off();
	}

	// ���ڸ������ݵ�������
	UpdateUiPeriod();

	// printf("[%s%d][%fmbar]\n", __func__, __LINE__, g_fV_mbar);
	// sprintf(sendBuffer, "*RP?:");
	// int ret = Frame_CheckSum_(sendBuffer, strlen(sendBuffer));
	// printf("[%s%d][%s][%d]\n", __func__, __LINE__, sendBuffer, ret);
	// sprintf(sendBuffer, "*IZ:");
	// ret = Frame_CheckSum_(sendBuffer, strlen(sendBuffer));
	// printf("[%s%d][%s][%d]\n", __func__, __LINE__, sendBuffer, ret);
}
// 1ms�ж��¼�
void Func_Task_Interrupt(void)
{
	static int flag = 1;
	if (flag)
	{
		flag = 0;
		GPIO_SetBits(GPIOA, GPIO_Pin_5); // PA5
	}
	else
	{
		flag = 1;
		GPIO_ResetBits(GPIOA, GPIO_Pin_5); // PA5
	}
}
/*****************************callback end***************************************************/
// ��������ʱ��		�첽����
volatile SoftTimer g_sTimerArray[] = {
	{0, 1, Func_Task_1ms01},
	{5, 9, Func_Task_10ms01},
	{37, 99, Func_Task_100ms01},
	{373, 999, Func_Task_1000ms01},
};

// Systik��ʱ���жϸ��¶�ʱ�� 10ms����һ��
void UpdataSoftTimer(void)
{
	Func_Task_Interrupt(); // �жϻص�������
	for (uint32_t i = 0; i < sizeof(g_sTimerArray) / sizeof(SoftTimer); i++)
	{ // ������ʱ��
		g_sTimerArray[i].m_nCounter++;
	}
}

// ��ʱ���������
void TaskSchedule(void)
{
	for (uint32_t i = 0; i < sizeof(g_sTimerArray) / sizeof(SoftTimer); i++)
	{ // ������ʱ��
		if (g_sTimerArray[i].m_nCounter > g_sTimerArray[i].m_nMaxCounter)
		{
			g_sTimerArray[i].m_nCounter = 0;
			(*(g_sTimerArray[i].funcCallBack))();
		}
	}
}

extern int main1(void);
int main(void)
{
	uint8_t byte;
	RCC_Config();
	NVIC_Config();
	LED1_Config();
	LED1_On();
	USART1_Config();
	USART2_Config();
	USART3_Config();
	TIM2_Config(); // 1KHZ

	USART1_SendByte(0x01);
	// USART2_SendByte(0x02);
	USART2_SendStr(SENSOR_COMMAND_RP, strlen(SENSOR_COMMAND_RP));

	USART3_SendByte(0x01); // ��һ���ֽڷ����쳣
	// USART3_SendByte(0x02);
	// USART3_SendByte(0x03);
	
	LoadBasicParamFromFlash(GetBasicParamHandle()); // ��Flash�ж�ȡ��������
	UpdateUiInit(); // ��ʼ��������UI

	USART_GPIO_Init();	  // ��ʼ������GPIO
	GlobalBasicParam*p_sGlobalBasicParam=(void *)GetBasicParamHandle();
	Timer3_Init(p_sGlobalBasicParam->m_nBaudRate, p_sGlobalBasicParam->m_nWordLength	); // ��ʼ����ʱ��3
	Uart_SendByte(0x04);
	printf("Init Done\n");
	// extern void Flash_Write_Read_Example(void) ;
	// Flash_Write_Read_Example();
	// main1();
	while (1)
	{
		// ���UART1���յ�1֡����
		if (UART1_ReceiveState == 1) // ������
		{
			UART1_ReceiveState = 0;
			UART1_Frame_Handler(USART1, UART1_RxBuffer, UART1_RxCount);
			UART1_RxCount = 0;
		}
		// ���UART2���յ�1֡����
		if (UART2_ReceiveState == 1) // ������
		{
			UART2_ReceiveState = 0;
			UART2_Frame_Handler(USART2, UART2_RxBuffer, UART2_RxCount);
			UART2_RxCount = 0;
		}
		// ���UART2���յ�1֡����
		if (UART_IO_ReceiveState == 1) // �ⲿIO
		{
			UART_IO_Frame_Handler(NULL, UART_IO_RxBuffer, UART_IO_RxCount);
			UART_IO_RxCount = 0;
			UART_IO_ReceiveState = 0;
		}
		TaskSchedule();
		// if(GetUartIOCounter()>0)	{
		// 	printf("GetUartIOCounter:%d\n",GetUartIOCounter());
		// 	printf("Received data: %d\n",Uart_ReceiveByte());
		// }
	}
}
