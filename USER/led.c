#include "stm32f10x.h"
#include "led.h"
#include "uart.h"

/*******************************************************************************
 * Function Name : USART1_Config
 * Description   : LED1开启
 * Return        : None
 *******************************************************************************/
void BoardLED1_Config(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitTypeDef gpioInit;
	gpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	gpioInit.GPIO_Pin = GPIO_Pin_1;
	gpioInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &gpioInit);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	gpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	gpioInit.GPIO_Pin = GPIO_Pin_5;
	gpioInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &gpioInit);

	gpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	gpioInit.GPIO_Pin = GPIO_Pin_6;
	gpioInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &gpioInit);

	gpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	gpioInit.GPIO_Pin = GPIO_Pin_7;
	gpioInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &gpioInit);
}

void ShowLedConfig(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef gpioInit;
	gpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	gpioInit.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	gpioInit.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &gpioInit);
}
void ShowRedLed(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_5);
	GPIO_SetBits(GPIOA, GPIO_Pin_6);
}
void ShowGreenLed(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_5);
	GPIO_ResetBits(GPIOA, GPIO_Pin_6);
}
// 打开远程指示灯
void OpenRemoteLed(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_4);
}
// 关闭远程指示灯
void CloseRemoteLed(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_4);
}
// 打开电磁阀
void OpenValve(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_7);
}
// 关闭电磁阀
void CloseValve(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_7);
}
/*******************************************************************************
 * Function Name : LED1_On
 * Description   : LED1开启
 * Return        : None
 *******************************************************************************/
void BoardLED1_On(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_1);
}

/*******************************************************************************
 * Function Name : LED1_Off
 * Description   : LED1开启
 * Return        : None
 *******************************************************************************/
// LED1关闭
void BoardLED1_Off(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_1);
}

void ADC_Configuration(void)
{
	ADC_InitTypeDef ADC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	// 开启ADC和GPIOA的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA, ENABLE);

	// 配置PA0、PA1为模拟输入模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// ADC配置
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	// 使能ADC1
	ADC_Cmd(ADC1, ENABLE);

	// 校准ADC
	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1))
		;
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1))
		;
}

uint16_t Get_ADC_Value(uint8_t channel)
{
	ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_55Cycles5);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC))
		;
	return ADC_GetConversionValue(ADC1);
}
float ADC_Value_To_Voltage(uint16_t adcValue)
{
	// 假设参考电压为3.3V，12位ADC分辨率
	return (float)adcValue * 3.3 / 4096;
}

// 将ADC值转换为实际电压值的函数，考虑电阻分压情况（1/5分压）
float ADC_Value_To_ActualVoltage(uint16_t adcValue)
{
	// 假设参考电压为3.3V，12位ADC分辨率，经过1/5电阻分压
	return (float)adcValue * 3.3 * 5 / 4096;
}

// 函数1：获取PA0（电池）和PA1（充电器）的实际电压值（考虑分压后还原）
void Get_ActualVoltages(float *voltagePA0, float *voltagePA1)
{
	uint16_t adcValuePA0 = Get_ADC_Value(ADC_Channel_0);
	uint16_t adcValuePA1 = Get_ADC_Value(ADC_Channel_1);

	*voltagePA0 = ADC_Value_To_ActualVoltage(adcValuePA0);
	*voltagePA1 = ADC_Value_To_ActualVoltage(adcValuePA1);
}

// 函数2：判断电池是否正在充电
uint8_t Is_Charging(float voltagePA0, float voltagePA1)
{
	// 简单判断，如果充电器电压大于电池电压 + 0.1V，认为在充电（可根据实际情况调整阈值等判断逻辑）
	if (voltagePA1 > voltagePA0 + 0.1)
	{
		return 1;
	}
	return 0;
}

// 函数3：采用分段线性方式获取电池电量
int Get_Battery_Power(float voltagePA0)
{
	float singleCellVoltage = voltagePA0 / 3; // 计算单节电池电压

	// 定义各电量区间对应的电压阈值及斜率
	const float voltageThresholds[] = {3.0, 3.6, 4.0, 4.2};
	const float slopes[] = {30.0 / (3.6 - 3.0), 30.0 / (4.0 - 3.6), 20.0 / (4.2 - 4.0)};

	int batteryPower = 1;
	if (singleCellVoltage < voltageThresholds[0])
	{
		batteryPower = 1;
	}
	else if (singleCellVoltage < voltageThresholds[1])
	{
		batteryPower = (int)((singleCellVoltage - voltageThresholds[0]) * slopes[0]);
	}
	else if (singleCellVoltage < voltageThresholds[2])
	{
		batteryPower = 30 + (int)((singleCellVoltage - voltageThresholds[1]) * slopes[1]);
	}
	else if (singleCellVoltage < voltageThresholds[3])
	{
		batteryPower = 60 + (int)((singleCellVoltage - voltageThresholds[2]) * slopes[2]);
	}
	else
	{
		batteryPower = 100;
	}

	return batteryPower;
}
// 更新UI显示电池电量及充电状态
void updateBatteryInfo2UI(int batteryPower, uint8_t isCharging)
{
	char tmpBuffer[128];
	static uint8_t oldIsCharging = 1;
	static uint8_t oldBatteryPower = 1;

	if (oldIsCharging != isCharging)
	{
		oldIsCharging = isCharging;
		memset(tmpBuffer, 0, 128); // 控制充电动画
		sprintf((char *)tmpBuffer, "vis gm0,\" %d\"\xff\xff\xff", isCharging);
		USART1_SendStr(tmpBuffer, strlen(tmpBuffer));

		memset(tmpBuffer, 0, 128); // 播放音乐
		sprintf(tmpBuffer, "beep 100\xff\xff\xff");
		USART1_SendStr(tmpBuffer, strlen(tmpBuffer));
	}

	if (oldBatteryPower != batteryPower)
	{
		oldBatteryPower = batteryPower;
		memset(tmpBuffer, 0, 128); // 更新电池电压
		sprintf(tmpBuffer, "j0.val=\" %d\"\xff\xff\xff", batteryPower);

		memset(tmpBuffer, 0, 128); // 更新电池电压
		sprintf(tmpBuffer, "click n0,0\xff\xff\xff");
		USART1_SendStr(tmpBuffer, strlen(tmpBuffer));
	}
}
// 3S更新一次
void BatteryManagementTask(void)
{
	float voltagePA0, voltagePA1;
	Get_ActualVoltages(&voltagePA0, &voltagePA1);
	uint8_t isCharging = 0;
	isCharging = Is_Charging(voltagePA0, voltagePA1);
	int batteryPower = 0;
	batteryPower = Get_Battery_Power(voltagePA0);

	printf("Battery Power: %d%%\n", batteryPower);
	printf("Charging: %s\n", isCharging ? "Yes" : "No");
	updateBatteryInfo2UI(batteryPower, isCharging);
}
