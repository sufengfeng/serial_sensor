#include "stm32f10x.h"
#include "led.h"
#include "uart.h"

/*******************************************************************************
 * Function Name : USART1_Config
 * Description   : LED1����
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
// ��Զ��ָʾ��
void OpenRemoteLed(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_4);
}
// �ر�Զ��ָʾ��
void CloseRemoteLed(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_4);
}
// �򿪵�ŷ�
void OpenValve(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_7);
}
// �رյ�ŷ�
void CloseValve(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_7);
}
/*******************************************************************************
 * Function Name : LED1_On
 * Description   : LED1����
 * Return        : None
 *******************************************************************************/
void BoardLED1_On(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_1);
}

/*******************************************************************************
 * Function Name : LED1_Off
 * Description   : LED1����
 * Return        : None
 *******************************************************************************/
// LED1�ر�
void BoardLED1_Off(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_1);
}

void ADC_Configuration(void)
{
	ADC_InitTypeDef ADC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	// ����ADC��GPIOA��ʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA, ENABLE);

	// ����PA0��PA1Ϊģ������ģʽ
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// ADC����
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	// ʹ��ADC1
	ADC_Cmd(ADC1, ENABLE);

	// У׼ADC
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
	// ����ο���ѹΪ3.3V��12λADC�ֱ���
	return (float)adcValue * 3.3 / 4096;
}

// ��ADCֵת��Ϊʵ�ʵ�ѹֵ�ĺ��������ǵ����ѹ�����1/5��ѹ��
float ADC_Value_To_ActualVoltage(uint16_t adcValue)
{
	// ����ο���ѹΪ3.3V��12λADC�ֱ��ʣ�����1/5�����ѹ
	return (float)adcValue * 3.3 * 5 / 4096;
}

// ����1����ȡPA0����أ���PA1�����������ʵ�ʵ�ѹֵ�����Ƿ�ѹ��ԭ��
void Get_ActualVoltages(float *voltagePA0, float *voltagePA1)
{
	uint16_t adcValuePA0 = Get_ADC_Value(ADC_Channel_0);
	uint16_t adcValuePA1 = Get_ADC_Value(ADC_Channel_1);

	*voltagePA0 = ADC_Value_To_ActualVoltage(adcValuePA0);
	*voltagePA1 = ADC_Value_To_ActualVoltage(adcValuePA1);
}

// ����2���жϵ���Ƿ����ڳ��
uint8_t Is_Charging(float voltagePA0, float voltagePA1)
{
	// ���жϣ�����������ѹ���ڵ�ص�ѹ + 0.1V����Ϊ�ڳ�磨�ɸ���ʵ�����������ֵ���ж��߼���
	if (voltagePA1 > voltagePA0 + 0.1)
	{
		return 1;
	}
	return 0;
}

// ����3�����÷ֶ����Է�ʽ��ȡ��ص���
int Get_Battery_Power(float voltagePA0)
{
	float singleCellVoltage = voltagePA0 / 3; // ���㵥�ڵ�ص�ѹ

	// ��������������Ӧ�ĵ�ѹ��ֵ��б��
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
// ����UI��ʾ��ص��������״̬
void updateBatteryInfo2UI(int batteryPower, uint8_t isCharging)
{
	char tmpBuffer[128];
	static uint8_t oldIsCharging = 1;
	static uint8_t oldBatteryPower = 1;

	if (oldIsCharging != isCharging)
	{
		oldIsCharging = isCharging;
		memset(tmpBuffer, 0, 128); // ���Ƴ�綯��
		sprintf((char *)tmpBuffer, "vis gm0,\" %d\"\xff\xff\xff", isCharging);
		USART1_SendStr(tmpBuffer, strlen(tmpBuffer));

		memset(tmpBuffer, 0, 128); // ��������
		sprintf(tmpBuffer, "beep 100\xff\xff\xff");
		USART1_SendStr(tmpBuffer, strlen(tmpBuffer));
	}

	if (oldBatteryPower != batteryPower)
	{
		oldBatteryPower = batteryPower;
		memset(tmpBuffer, 0, 128); // ���µ�ص�ѹ
		sprintf(tmpBuffer, "j0.val=\" %d\"\xff\xff\xff", batteryPower);

		memset(tmpBuffer, 0, 128); // ���µ�ص�ѹ
		sprintf(tmpBuffer, "click n0,0\xff\xff\xff");
		USART1_SendStr(tmpBuffer, strlen(tmpBuffer));
	}
}
// 3S����һ��
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
