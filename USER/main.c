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

float sliding_window_filter_max_min(float current_value)
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
	float max_val = current_value; // ��ʼ��Ϊ��С������
	float min_val = current_value; // ��ʼ��Ϊ��󸡵���

	for (int i = 0; i < count; i++)
	{
		if (history_data[i] > max_val)
		{
			max_val = history_data[i];
		}
		if (history_data[i] < min_val)
		{
			min_val = history_data[i];
		}
		sum += history_data[i];
	}

	// ��ȥ���ֵ����Сֵ
	sum -= (max_val + min_val);

	// ����ƽ��ֵ����ĸΪ���ڴ�С��ȥ2
	return sum / (count - 2);
}

uint8_t PACE_COMMAND_PRESSURE[] = {0x3A, 0x53, 0x65, 0x6E, 0x73, 0x3F, 0x0D, 0x0A};
uint8_t PACE_RESULT_HEAD[] = ":SEHS:PRES";

#define SENSOR_COMMAND_RP "*RP?:25\r\n" // ��ѯѹ��ֵ
#define SENSOR_COMMAND_AZ "*IZ:63\r\n"	// �Զ�У��
// [Func_Task_1000ms01303][*RP?:][25]
// [Func_Task_1000ms01306][*IZ:][63]
float g_fV_mbar = 0;
float g_fV_psi = 0;				 // psiѹ��ֵ
float g_fV_rate = 0;			 // psiѹ��ֵ�仯��
float g_fV_rateMax = 0.00145;	 // psiѹ��ֵ�仯�����ֵ
uint8_t g_bIsAutoZero = 0;		 // �Զ�У���־λ
uint8_t g_bIsAutoZeroReason = 0; // �Զ�У��ԭ��	0���ޣ�bit1�������У�bit2������
uint8_t g_bFlageStatus = 0;		 // Զ��ģʽ��־λ

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
		// g_fV_mbar = sliding_window_filter_max_min(tmpValue);
		// printf("[%s%d][%f]\n", __func__, __LINE__, g_fV_mbar);
	}
}

#define PR_COMMAND_CLS "*CLS\r\n"			   // ����
#define PR_COMMAND_LOCAL "LOCAL\r\n"		   // ����Ϊ����ģʽ
#define PR_COMMAND_REMOTE "REMOTE\r\n"		   // ����ΪԶ��ģʽ
#define PR_COMMAND_CONF1 "CONF1"			   // ����Ϊ����ģʽ
#define PR_COMMAND_COM_PARAMETER "COM1 "	   // ���ô��ڲ���
#define PR_COMMAND_AUTOZERO "AUTOZERO=RUN\r\n" // �Զ�У��UTOZERO
#define PR_COMMAND_RATE "RATE\r\n"			   // ��ѯ������
#define PR_COMMAND_PR "PR\r\n"				   // ��ѯѹ��ֵ
#define PR_COMMAND_PR_ "PR?\r\n"			   // ��ѯѹ��ֵ

#define PR_COMMAND_pr "pr\r\n"	 // ��ѯѹ��ֵ
#define PR_COMMAND_pr_ "pr?\r\n" // ��ѯѹ��ֵ

#define PR_COMMAND_READ "READ?\r\n"			  // ��ѯѹ��ֵ
#define PR_COMMAND_READ1 "READ1?\r\n"		  // ��ѯѹ��ֵ
#define COMAND_COM1_SET "COM1 2400,E,7,1\r\n" // ���ô��ڲ���
#define COMAND_COM1_SET_RES "2400,E,7,1\r\n"

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
#include "math.h"

void ReponceComandPR(void)
{
	char sendBuffer[128];
	memset(sendBuffer, 0, 128);
	float bmpValue = fabs(g_fV_rate);
	if (bmpValue > g_fV_rateMax)
	{
		sprintf(sendBuffer, "NR      %.3f psi g\r\n", g_fV_psi);
		Uart_SendByteStr(sendBuffer, strlen(sendBuffer));
	}
	else
	{
		sprintf(sendBuffer, "R       %.3f psi g\r\n", g_fV_psi);
		Uart_SendByteStr(sendBuffer, strlen(sendBuffer));
	}
}
void ReponceComandREAD(void)
{
	char sendBuffer[128];
	memset(sendBuffer, 0, 128);
	sprintf(sendBuffer, "%.3f\r\n", g_fV_psi);
	Uart_SendByteStr(sendBuffer, strlen(sendBuffer));
}

/*******************************************************************************
 * Function Name : UART_IO_Frame_Handler
 * Description   : ��������֡����ȡ��������֡����У�����ݣ�
 * Input         : None
 * Return        : None
 *******************************************************************************/
void UART_IO_Frame_Handler(USART_TypeDef *USARTtype, volatile uint8_t buffer[], volatile uint8_t len)
{
	printf("[%s%d][%s][%d][%d]\n", __func__, __LINE__, buffer, len,buffer[0]);
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
		if (g_bFlageStatus)
		{ // ��ʼ��
			g_bFlageStatus = 0;
		}
		else
		{
			g_bFlageStatus = 1;
		}
		Uart_SendByteStr(buffer, len);
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_AUTOZERO, strlen(PR_COMMAND_AUTOZERO))) // �Զ�У��
	{

		g_bIsAutoZero = 1;
		g_bIsAutoZeroReason = g_bIsAutoZeroReason | 0x20;
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_CONF1, strlen(PR_COMMAND_CONF1))) // �Զ�У��
	{
		Uart_SendByteStr(PR_RESPONE_OK, strlen(PR_RESPONE_OK));
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_RATE, strlen(PR_COMMAND_RATE))) // ��ѯ������
	{
		char sendBuffer[128];
		memset(sendBuffer, 0, 128);
		sprintf(sendBuffer, "%f psi/s\r\n", g_fV_rate);
		Uart_SendByteStr(sendBuffer, strlen(sendBuffer));
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_PR, strlen(PR_COMMAND_PR)))
	{
		ReponceComandPR();
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_PR_, strlen(PR_COMMAND_PR_)))
	{
		ReponceComandPR();
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_pr, strlen(PR_COMMAND_pr)))
	{
		ReponceComandPR();
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_pr_, strlen(PR_COMMAND_pr_)))
	{
		ReponceComandPR();
	}

	else if (!strncmp((const char *)buffer, PR_COMMAND_READ, strlen(PR_COMMAND_READ)))
	{
		ReponceComandPR();
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_READ1, strlen(PR_COMMAND_READ1)))
	{
		ReponceComandREAD();
	}
	else if (!strncmp((const char *)buffer, COMAND_COM1_SET, strlen(COMAND_COM1_SET)))
	{
		Uart_SendByteStr(COMAND_COM1_SET_RES, strlen(COMAND_COM1_SET_RES));
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
	if (!strncmp((const char *)buffer, UI_COMMAND_AZ, strlen(UI_COMMAND_AZ))) // �յ�����
	{
		g_bIsAutoZero = 1;
		g_bIsAutoZeroReason = g_bIsAutoZeroReason | 0x01;
	}
	else if (!strncmp((const char *)buffer, "BaudRate=", strlen("BaudRate="))) // BaudRate=9600;WordLength=8;StopBits=1;Parity=None
	{
		GlobalBasicParam *p_sGlobalBasicParam = (void *)GetBasicParamHandle();
		char tmpBuffer[128];
		sscanf((const char *)buffer, "BaudRate=%d;WordLength=%d;StopBits=%d;Parity=%s", &(p_sGlobalBasicParam->m_nBaudRate), &(p_sGlobalBasicParam->m_nWordLength), &(p_sGlobalBasicParam->m_nStopBits), tmpBuffer);
		if (!strncmp((const char *)tmpBuffer, "None", strlen("None")))
		{
			p_sGlobalBasicParam->m_nParity = enumNone;
		}
		else if (!strncmp((const char *)tmpBuffer, "Odd", strlen("Odd")))
		{
			p_sGlobalBasicParam->m_nParity = enumOdd;
		}
		else
		{
			p_sGlobalBasicParam->m_nParity = enumEven;
		}
		PrintBasicParam(p_sGlobalBasicParam);
	}
	else if (!strncmp((const char *)buffer, "SaveSerialParam", strlen("SaveSerialParam"))) //"SaveSerialParam"	���洮�ڲ���
	{
		printf("[%s][%d]SaveSerialParam\n", __func__, __LINE__);
		SaveCurrentBasicParam();
		Reboot();
	}
	else if (!strncmp((const char *)buffer, "page_com_setting", strlen("page_com_setting"))) //"page_com_setting"	���洮�ڲ���
	{
		UpdateUiInit(); //
	}
}
void ControlRemoteStatue(void) // �����Զ�У��
{
	if (g_bFlageStatus)
	{
		OpenRemoteLed();
	}
	else
	{
		CloseRemoteLed();
	}
}
void Show_OverPress(void){

}
#define MAX_MPA_PROTECT (2100)
void ControlAutoZero(void) // �����Զ�У��
{
	static uint8_t stepZero = 0;
	if (g_bIsAutoZero == 1) // �����ǰ���Զ�У��	״̬	��ʼ�Զ�У��	״̬	�����򲻴���
	{
		switch (stepZero)
		{
		case 0:
		{
			printf("[%s][%d]Begin AutoZero ...\n", __func__, __LINE__);
			OpenValve(); // ��У�㷧
			stepZero = 1;
		}
		break;
		case 1:
		{
			static int counter = 0;
			if (counter++ >= 6)
			{ // �ȴ�600ms
				counter = 0;
				stepZero = 2;
			}
		}
		break;
		case 2:
		{
			SendComandAutoZero(); // ����У������
			stepZero = 3;
			break;
		}
		case 3:
		{
			static int counter = 0;
			if (counter++ >= 4)
			{ // �ȴ�400ms
				counter = 0;
				stepZero = 4;
			}
		}
		break;
		case 4:
		{
			CloseValve();
			stepZero = 0;
			g_bIsAutoZero = 0;
			if (g_bIsAutoZeroReason & 0x01) // ����
			{
				printf("[%s][%d]AutoZero show Done!!\n", __func__, __LINE__);
			}
			else if (g_bIsAutoZeroReason & 0x02)
			{
				Uart_SendByteStr(PR_RESPONE_OK, strlen(PR_RESPONE_OK)); // ����У��������Ӧ
				printf("[%s][%d]AutoZero command Done!!\n", __func__, __LINE__);
			}
			else
			{
				printf("[%s][%d]AutoZero Done!!\n", __func__, __LINE__);
			}
			g_bIsAutoZeroReason = 0;
		}
		break;
		default:
		{
			stepZero = 0;
			g_bIsAutoZero = 0;
			g_bIsAutoZeroReason = 0;
		}

		break;
		}
	}
	if(g_fV_mbar>MAX_MPA_PROTECT){
		Show_OverPress();
		CloseValve();
	}
}
void ControlShowLed(void ){
	if (g_fV_rate >= g_fV_rateMax)
	{
		ShowRedLed();
	}
	else
	{
		ShowGreenLed();
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
	ControlAutoZero();	// �����Զ�У��
	// Uart_SendByte('O');
	//Uart_SendByte(0x5A);
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
	g_fV_psi = 0.0145037744 * g_fV_mbar;
	if (g_fV_psi >= 0)
	{
		sprintf(sendBuffer, "home_page0.t0.txt=\" %.5f\"\xff\xff\xff", g_fV_psi);
	}
	else
	{
		sprintf(sendBuffer, "home_page0.t0.txt=\"%.5f\"\xff\xff\xff", g_fV_psi);
	}

	USART1_SendStr(sendBuffer, strlen(sendBuffer));

	g_fV_rate = g_fV_psi - last_V_psi;
	if (g_fV_rate >= 0)
	{
		sprintf(sendBuffer, "home_page0.t3.txt=\" %.5f\"\xff\xff\xff", g_fV_rate);
	}
	else
	{
		sprintf(sendBuffer, "home_page0.t3.txt=\"%.5f\"\xff\xff\xff", g_fV_rate);
	}
	USART1_SendStr(sendBuffer, strlen(sendBuffer));
}
// ���ڸ������ݵ�������
int UpdateUiInit(void)
{
	char sendBuffer[128];
	char tmpBuffer[128];
	memset(sendBuffer, 0, 128);
	GlobalBasicParam *p_sGlobalBasicParam = (void *)GetBasicParamHandle();
	// sprintf(sendBuffer, "setting.cb0.txt=\"%d\" \xff\xff\xff", p_sGlobalBasicParam->m_nBaudRate);	// ������
	sprintf(sendBuffer, "com_setting.cb0.txt=\" %d\"\xff\xff\xff", p_sGlobalBasicParam->m_nBaudRate);
	USART1_SendStr(sendBuffer, strlen(sendBuffer));

	memset(sendBuffer, 0, 128);
	sprintf(sendBuffer, "com_setting.cb1.txt=\"%d\"\xff\xff\xff", p_sGlobalBasicParam->m_nWordLength); // ����λ
	USART1_SendStr(sendBuffer, strlen(sendBuffer));

	memset(sendBuffer, 0, 128);
	sprintf(sendBuffer, "com_setting.cb2.txt=\"%d\"\xff\xff\xff", p_sGlobalBasicParam->m_nStopBits); // ֹͣλ
	USART1_SendStr(sendBuffer, strlen(sendBuffer));

	if (p_sGlobalBasicParam->m_nParity == enumNone)
	{
		sprintf(tmpBuffer, "None");
	}
	else if (p_sGlobalBasicParam->m_nParity == enumOdd)
	{
		sprintf(tmpBuffer, "Odd");
	}
	else
	{
		sprintf(tmpBuffer, "Even");
	}
	memset(sendBuffer, 0, 128);
	sprintf(sendBuffer, "com_setting.cb3.txt=\"%s\"\xff\xff\xff", tmpBuffer); // ��żУ��
	USART1_SendStr(sendBuffer, strlen(sendBuffer));
}
void	TriggerBoardLed(void)	{
	static uint8_t flag = 0;
	if (flag)
	{
		flag = 0;
		BoardLED1_On();
	}
	else
	{
		flag = 1;
		BoardLED1_Off();
	}
}
// 1000ms�ص��¼�
void Func_Task_1000ms01(void)
{
	TriggerBoardLed();
	UpdateUiPeriod();	// ���ڸ������ݵ�������
	ControlShowLed(); // ���������Ƿ��ȶ���ʾ��
	ControlRemoteStatue();	// Զ�̿���״̬��ʾ
	static uint8_t counter3s = 0; // 3s������
	if (++counter3s >= 3)
    {
		counter3s = 0;
		BatteryManagementTask(); // ��ع�������
	}
}
// 1ms�ж��¼�
void Func_Task_Interrupt(void)
{
	// static int flag = 1;
	// if (flag)
	// {
	// 	flag = 0;
	// 	GPIO_SetBits(GPIOA, GPIO_Pin_5); // PA5
	// }
	// else
	// {
	// 	flag = 1;
	// 	GPIO_ResetBits(GPIOA, GPIO_Pin_5); // PA5
	// }
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
	BoardLED1_Config();
	BoardLED1_On();
	ShowLedConfig();
	USART1_Config();
	USART2_Config();
	USART3_Config();
	TIM2_Config(); // 1KHZ
	ADC_Configuration();	// ADC��ʼ��

	USART1_SendByte(0x01);
	// USART2_SendByte(0x02);
	USART2_SendStr(SENSOR_COMMAND_RP, strlen(SENSOR_COMMAND_RP));

	USART3_SendByte(0x01); // ��һ���ֽڷ����쳣

	// USART3_SendByte(0x02);
	// USART3_SendByte(0x03);

	LoadBasicParamFromFlash(GetBasicParamHandle()); // ��Flash�ж�ȡ��������
	UpdateUiInit();									// ��ʼ��������UI

	USART_GPIO_Init(); // ��ʼ������GPIO
	GlobalBasicParam *p_sGlobalBasicParam = (void *)GetBasicParamHandle();
	// Timer3_Init(p_sGlobalBasicParam->m_nBaudRate, p_sGlobalBasicParam->m_nWordLength,0,1); // ��ʼ����ʱ��3
	Timer3_Init(9600, 8,0,1); // ��ʼ����ʱ��3
	// Uart_SendByte(0x04);
	// Uart_SendByteStr("OK");
	printf("Init Done\n");
	LOG(LOG_CRIT, "\n\rCopyright (c) 2021,Geekplus All rights reserved.\n\rRelease SafePLC version=[0x%08lx] %s-%s\r\n", p_sGlobalBasicParam->m_nAppVersion, __DATE__, __TIME__);
	// extern void Flash_Write_Read_Example(void) ;
	// Flash_Write_Read_Example();
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
