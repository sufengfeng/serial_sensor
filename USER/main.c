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
 * Description    : 外设配置
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
 * Description   : 中断配置
 * Input         : None
 * Return        : None
 *******************************************************************************/
void NVIC_Config(void)
{
	// 设置NVIC中断分组2:2位抢占优先级，2位响应优先级
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
	for (i = 0; i < length; i++)
	{
		res += buffer[i];
	}
	res = ~res + 1;
	return res;
}

/*******************************************************************************
 * Function Name : Frame_Handler
 * Description   : 处理数据帧（提取完整数据帧，并校正数据）
 * Input         : None
 * Return        : None
 *******************************************************************************/
void Frame_Handler_(USART_TypeDef *USARTin, USART_TypeDef *USARTout, volatile uint8_t buffer[], volatile uint8_t count)
{
	uint8_t frameLength = 21;
	// 判断是否是取数返回的数据包([0]包头，[1]M(0x4D),ASCII编码，[2~6]水平偏差，[7~11]垂直偏差，[12~15]光强，[16~19]灯高,[20]校验码)
	if (frameLength == count && buffer[0] == 0x02 && buffer[1] == 0x4D)
	{
		// 计算校验码
		uint8_t crc = Frame_CheckSum(buffer, frameLength - 1);
		// 验证校验码
		if (crc == buffer[frameLength - 1])
		{
			// 随机产生一个负100-200的数值替换。例如设置为：-[0x2D]1[0x31]5[0x35]0[0x30]空格[0x20]
			buffer[8] = 0x2D;
			buffer[8] = 0x31;
			buffer[9] = 0x35;
			buffer[10] = 0x30;
			buffer[11] = 0x20;
			// 光强如果小于180且大于0，随机产生一个180-380的数值替换。例如设置为：0[0x30]2[0x32]0[0x30]0[0x30]
			buffer[12] = 0x30;
			buffer[13] = 0x32;
			buffer[14] = 0x30;
			buffer[15] = 0x30;
			// 重新设置校验码
			buffer[frameLength - 1] = Frame_CheckSum(buffer, frameLength - 1);
		}
	}
	// 发送数据
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
	float max_val = current_value; // 初始化为最小浮点数
	float min_val = current_value; // 初始化为最大浮点数

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

	// 减去最大值和最小值
	sum -= (max_val + min_val);

	// 计算平均值，分母为窗口大小减去2
	return sum / (count - 2);
}

uint8_t PACE_COMMAND_PRESSURE[] = {0x3A, 0x53, 0x65, 0x6E, 0x73, 0x3F, 0x0D, 0x0A};
uint8_t PACE_RESULT_HEAD[] = ":SEHS:PRES";

#define SENSOR_COMMAND_RP "*RP?:25\r\n" // 查询压力值
#define SENSOR_COMMAND_AZ "*IZ:63\r\n"	// 自动校零
// [Func_Task_1000ms01303][*RP?:][25]
// [Func_Task_1000ms01306][*IZ:][63]
float g_fV_mbar = 0;
float g_fV_psi = 0;				 // psi压力值
float g_fV_rate = 0;			 // psi压力值变化率
float g_fV_rateMax = 0.00145;	 // psi压力值变化率最大值
uint8_t g_bIsAutoZero = 0;		 // 自动校零标志位
uint8_t g_bIsAutoZeroReason = 0; // 自动校零原因	0：无，bit1：命令行，bit2：界面
uint8_t g_bFlageStatus = 0;		 // 远程模式标志位

/*******************************************************************************
 * Function Name : UART2_Frame_Handler
 * Description   : 处理数据帧（提取完整数据帧，并校正数据）
 * Input         : None
 * Return        : None
 *******************************************************************************/
void UART2_Frame_Handler(USART_TypeDef *USARTtype, volatile uint8_t buffer[], volatile uint8_t count)
{
	// printf("[%s%d][%s][%d]\n", __func__, __LINE__, buffer, count);
	if (strncmp((const char *)buffer, "!rp=", strlen("!rp=")) <= 0) // 收到命令	"!rp=-0.337:74\r\n"
	{
		float tmpValue = 0;
		sscanf((const char *)buffer, "!rp=%f", &tmpValue);
		g_fV_mbar = sliding_window_filter(tmpValue);
		// g_fV_mbar = sliding_window_filter_max_min(tmpValue);
		// printf("[%s%d][%f]\n", __func__, __LINE__, g_fV_mbar);
	}
}

#define PR_COMMAND_CLS "*CLS\r\n"			   // 清屏
#define PR_COMMAND_LOCAL "LOCAL\r\n"		   // 设置为本地模式
#define PR_COMMAND_REMOTE "REMOTE\r\n"		   // 设置为远程模式
#define PR_COMMAND_CONF1 "CONF1"			   // 设置为本地模式
#define PR_COMMAND_COM_PARAMETER "COM1 "	   // 设置串口参数
#define PR_COMMAND_AUTOZERO "AUTOZERO=RUN\r\n" // 自动校零UTOZERO
#define PR_COMMAND_RATE "RATE\r\n"			   // 查询采样率
#define PR_COMMAND_PR "PR\r\n"				   // 查询压力值
#define PR_COMMAND_PR_ "PR?\r\n"			   // 查询压力值

#define PR_COMMAND_pr "pr\r\n"	 // 查询压力值
#define PR_COMMAND_pr_ "pr?\r\n" // 查询压力值

#define PR_COMMAND_READ "READ?\r\n"			  // 查询压力值
#define PR_COMMAND_READ1 "READ1?\r\n"		  // 查询压力值
#define COMAND_COM1_SET "COM1 2400,E,7,1\r\n" // 设置串口参数
#define COMAND_COM1_SET_RES "2400,E,7,1\r\n"

#define PR_RESPONE_OK "OK\r\n" // 响应OK

// 获取字符串校验和
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
 * Description   : 处理数据帧（提取完整数据帧，并校正数据）
 * Input         : None
 * Return        : None
 *******************************************************************************/
void UART_IO_Frame_Handler(USART_TypeDef *USARTtype, volatile uint8_t buffer[], volatile uint8_t len)
{
	printf("[%s%d][%s][%d][%d]\n", __func__, __LINE__, buffer, len,buffer[0]);
	if (!strncmp((const char *)buffer, PR_COMMAND_CLS, strlen(PR_COMMAND_CLS))) // 清屏
	{																			// 收到命令
		Uart_SendByteStr(PR_RESPONE_OK, strlen(PR_RESPONE_OK));
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_LOCAL, strlen(PR_COMMAND_LOCAL))) // 设置为本地模式
	{
		Uart_SendByteStr(buffer, len);
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_REMOTE, strlen(PR_COMMAND_REMOTE))) // 设置为远程模式
	{
		if (g_bFlageStatus)
		{ // 初始化
			g_bFlageStatus = 0;
		}
		else
		{
			g_bFlageStatus = 1;
		}
		Uart_SendByteStr(buffer, len);
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_AUTOZERO, strlen(PR_COMMAND_AUTOZERO))) // 自动校零
	{

		g_bIsAutoZero = 1;
		g_bIsAutoZeroReason = g_bIsAutoZeroReason | 0x20;
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_CONF1, strlen(PR_COMMAND_CONF1))) // 自动校零
	{
		Uart_SendByteStr(PR_RESPONE_OK, strlen(PR_RESPONE_OK));
	}
	else if (!strncmp((const char *)buffer, PR_COMMAND_RATE, strlen(PR_COMMAND_RATE))) // 查询采样率
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
#define UI_COMMAND_AZ "AutoZ" // 自动校零
/*******************************************************************************
 * Function Name : UART1_Frame_Handler
 * Description   : 处理数据帧（提取完整数据帧，并校正数据）
 * Input         : None
 * Return        : None
 *******************************************************************************/
void UART1_Frame_Handler(USART_TypeDef *USARTtype, volatile uint8_t buffer[], volatile uint8_t len)
{
	printf("[%s%d][%s][%d]\n", __func__, __LINE__, buffer, len);
	if (!strncmp((const char *)buffer, UI_COMMAND_AZ, strlen(UI_COMMAND_AZ))) // 收到命令
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
	else if (!strncmp((const char *)buffer, "SaveSerialParam", strlen("SaveSerialParam"))) //"SaveSerialParam"	保存串口参数
	{
		printf("[%s][%d]SaveSerialParam\n", __func__, __LINE__);
		SaveCurrentBasicParam();
		Reboot();
	}
	else if (!strncmp((const char *)buffer, "page_com_setting", strlen("page_com_setting"))) //"page_com_setting"	保存串口参数
	{
		UpdateUiInit(); //
	}
}
void ControlRemoteStatue(void) // 控制自动校零
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
void ControlAutoZero(void) // 控制自动校零
{
	static uint8_t stepZero = 0;
	if (g_bIsAutoZero == 1) // 如果当前在自动校零	状态	则开始自动校零	状态	，否则不处理
	{
		switch (stepZero)
		{
		case 0:
		{
			printf("[%s][%d]Begin AutoZero ...\n", __func__, __LINE__);
			OpenValve(); // 打开校零阀
			stepZero = 1;
		}
		break;
		case 1:
		{
			static int counter = 0;
			if (counter++ >= 6)
			{ // 等待600ms
				counter = 0;
				stepZero = 2;
			}
		}
		break;
		case 2:
		{
			SendComandAutoZero(); // 发送校零命令
			stepZero = 3;
			break;
		}
		case 3:
		{
			static int counter = 0;
			if (counter++ >= 4)
			{ // 等待400ms
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
			if (g_bIsAutoZeroReason & 0x01) // 界面
			{
				printf("[%s][%d]AutoZero show Done!!\n", __func__, __LINE__);
			}
			else if (g_bIsAutoZeroReason & 0x02)
			{
				Uart_SendByteStr(PR_RESPONE_OK, strlen(PR_RESPONE_OK)); // 发送校零命令响应
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
// 1ms回调事件
void Func_Task_1ms01(void)
{
}
// 10ms回调事件
void Func_Task_10ms01(void)
{
}
// 100ms回调事件
void Func_Task_100ms01(void)
{
	USART2_SendStr(SENSOR_COMMAND_RP, strlen(SENSOR_COMMAND_RP)); // 发送查询命令
	ControlAutoZero();	// 控制自动校零
	// Uart_SendByte('O');
	//Uart_SendByte(0x5A);
}
int SendComandAutoZero(void)
{
	return USART2_SendStr(SENSOR_COMMAND_AZ, strlen(SENSOR_COMMAND_AZ)); // 发送查询命令
}
// 周期更新数据到串口屏
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
// 周期更新数据到串口屏
int UpdateUiInit(void)
{
	char sendBuffer[128];
	char tmpBuffer[128];
	memset(sendBuffer, 0, 128);
	GlobalBasicParam *p_sGlobalBasicParam = (void *)GetBasicParamHandle();
	// sprintf(sendBuffer, "setting.cb0.txt=\"%d\" \xff\xff\xff", p_sGlobalBasicParam->m_nBaudRate);	// 波特率
	sprintf(sendBuffer, "com_setting.cb0.txt=\" %d\"\xff\xff\xff", p_sGlobalBasicParam->m_nBaudRate);
	USART1_SendStr(sendBuffer, strlen(sendBuffer));

	memset(sendBuffer, 0, 128);
	sprintf(sendBuffer, "com_setting.cb1.txt=\"%d\"\xff\xff\xff", p_sGlobalBasicParam->m_nWordLength); // 数据位
	USART1_SendStr(sendBuffer, strlen(sendBuffer));

	memset(sendBuffer, 0, 128);
	sprintf(sendBuffer, "com_setting.cb2.txt=\"%d\"\xff\xff\xff", p_sGlobalBasicParam->m_nStopBits); // 停止位
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
	sprintf(sendBuffer, "com_setting.cb3.txt=\"%s\"\xff\xff\xff", tmpBuffer); // 奇偶校验
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
// 1000ms回调事件
void Func_Task_1000ms01(void)
{
	TriggerBoardLed();
	UpdateUiPeriod();	// 周期更新数据到串口屏
	ControlShowLed(); // 控制气体是否稳定显示灯
	ControlRemoteStatue();	// 远程控制状态显示
	static uint8_t counter3s = 0; // 3s计数器
	if (++counter3s >= 3)
    {
		counter3s = 0;
		BatteryManagementTask(); // 电池管理任务
	}
}
// 1ms中断事件
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
// 多任务软定时器		异步任务
volatile SoftTimer g_sTimerArray[] = {
	{0, 1, Func_Task_1ms01},
	{5, 9, Func_Task_10ms01},
	{37, 99, Func_Task_100ms01},
	{373, 999, Func_Task_1000ms01},
};

// Systik定时器中断更新定时器 10ms调度一次
void UpdataSoftTimer(void)
{
	Func_Task_Interrupt(); // 中断回调服务函数
	for (uint32_t i = 0; i < sizeof(g_sTimerArray) / sizeof(SoftTimer); i++)
	{ // 更新软定时器
		g_sTimerArray[i].m_nCounter++;
	}
}

// 定时器任务调度
void TaskSchedule(void)
{
	for (uint32_t i = 0; i < sizeof(g_sTimerArray) / sizeof(SoftTimer); i++)
	{ // 更新软定时器
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
	ADC_Configuration();	// ADC初始化

	USART1_SendByte(0x01);
	// USART2_SendByte(0x02);
	USART2_SendStr(SENSOR_COMMAND_RP, strlen(SENSOR_COMMAND_RP));

	USART3_SendByte(0x01); // 第一个字节发送异常

	// USART3_SendByte(0x02);
	// USART3_SendByte(0x03);

	LoadBasicParamFromFlash(GetBasicParamHandle()); // 从Flash中读取基本参数
	UpdateUiInit();									// 初始化串口屏UI

	USART_GPIO_Init(); // 初始化串口GPIO
	GlobalBasicParam *p_sGlobalBasicParam = (void *)GetBasicParamHandle();
	// Timer3_Init(p_sGlobalBasicParam->m_nBaudRate, p_sGlobalBasicParam->m_nWordLength,0,1); // 初始化定时器3
	Timer3_Init(9600, 8,0,1); // 初始化定时器3
	// Uart_SendByte(0x04);
	// Uart_SendByteStr("OK");
	printf("Init Done\n");
	LOG(LOG_CRIT, "\n\rCopyright (c) 2021,Geekplus All rights reserved.\n\rRelease SafePLC version=[0x%08lx] %s-%s\r\n", p_sGlobalBasicParam->m_nAppVersion, __DATE__, __TIME__);
	// extern void Flash_Write_Read_Example(void) ;
	// Flash_Write_Read_Example();
	while (1)
	{
		// 如果UART1接收到1帧数据
		if (UART1_ReceiveState == 1) // 串口屏
		{
			UART1_ReceiveState = 0;
			UART1_Frame_Handler(USART1, UART1_RxBuffer, UART1_RxCount);
			UART1_RxCount = 0;
		}
		// 如果UART2接收到1帧数据
		if (UART2_ReceiveState == 1) // 传感器
		{
			UART2_ReceiveState = 0;
			UART2_Frame_Handler(USART2, UART2_RxBuffer, UART2_RxCount);
			UART2_RxCount = 0;
		}
		// 如果UART2接收到1帧数据
		if (UART_IO_ReceiveState == 1) // 外部IO
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
