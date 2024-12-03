#include "usart_io.h"
#include "stm32f10x.h"
#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "uart.h"
// ����һЩ����
// #define BAUD_RATE 9600
#define TIMER_PRESCALER 1
// #define TIMER_PERIOD ((72000000 / TIMER_PRESCALER) / BAUD_RATE)

// ���ͻ������ͽ��ջ�����
volatile uint8_t sendBuffer[8];
volatile uint8_t sendIndex = 0;

// ���ͺͽ���״̬��־
volatile uint8_t sending = 0;
volatile uint8_t receiving = 0;
extern uint8_t UART_IO_RxBuffer[128];
extern uint8_t UART_IO_RxCount;
extern uint8_t UART_IO_ReceiveState;

// void Timer3_Init(int BAUD_RATE);
void USART_GPIO_Init(void);
void Uart_SendByte(uint8_t data);
uint8_t Uart_ReceiveByte(void);
void TIM3_IRQHandler(void);
#define MAX_TIMER3_FILTER (5) // ÿλ��Ҫ��ʱ���ж�������
static int l_nUartWordLength = 8;

void Timer3_Init(int BAUD_RATE, int USART_WordLength)
{
    if (BAUD_RATE < 4800 || BAUD_RATE > 119200)
    {
        BAUD_RATE = 9600;
    }
    if (USART_WordLength < 7 || USART_WordLength > 9)
    {
        USART_WordLength = 8;
    }
    l_nUartWordLength = USART_WordLength; // ������Чλ
    // ������ʱ�� 3 ʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_Cmd(TIM3, DISABLE); // �رն�ʱ������ֹ��ͻ
    // int TIMER_PERIOD = (72000000 / BAUD_RATE)/MAX_TIMER3_FILTER;
    int TIMER_PERIOD = (14400000 / BAUD_RATE);
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period = TIMER_PERIOD - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = TIMER_PRESCALER - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    // ������ʱ���ж�
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    // ���� NVIC��ʹ�ܶ�ʱ���ж�
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    sendIndex = 0;
    UART_IO_RxCount = 0;
    UART_IO_ReceiveState = 0;
    // ������ʱ��
    TIM_Cmd(TIM3, ENABLE);
}

void USART_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // ���� GPIOA ʱ��
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // ���� PA9 Ϊ�������������ģ�⴮�ڷ���
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // �ⲿ�����������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // ���� PA10 Ϊ�������룬����ģ�⴮�ڽ���
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // �ڲ���������
    // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // �ⲿ��������
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}
void Uart_SendByteStr(uint8_t *str, int len)
{
    for (int i = 0; i < len; i++)
    {
        uint32_t counterTimeout = 0;
        while (sending)
        {
            if (counterTimeout++ > 1000000)
            {
                LOG(LOG_CRIT, "\r\nUart_SendByteStr  timeout\r\n");
                break;
            }
            // �ȴ��������
        }
        Uart_SendByte(str[i]);
    }
}

void Uart_SendByte(uint8_t data)
{
    // �����ݷ��뷢�ͻ�����
    sendBuffer[sendIndex++] = data;
    // �����ǰû�����ڷ��͵����ݣ����������͹���
    if (!sending)
    {
        sending = 1;
        TIM_SetCounter(TIM3, 0);
    }
}
// static int flag=0;
// if(flag==0) {
//     flag=1;
//     GPIO_SetBits(GPIOA, GPIO_Pin_9);
// }else {
//     flag=0;
//     GPIO_ResetBits(GPIOA, GPIO_Pin_9);
// }

// TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
// return ;
// MAX_TIMER3_FILTER

inline static int TX_SET_IO(int value)
{

    volatile static int call_count = 0; // ��̬�������ڼ�¼���������õĴ���
    // ÿ�ε���ʱ���Ӽ���
    call_count++;
    if (value == 1)
    { // ����value����A9���ŵĵ�ƽ
        GPIO_SetBits(GPIOA, GPIO_Pin_9);
    }
    else
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_9);
    }
    if (call_count < MAX_TIMER3_FILTER)
    {
        return 0; // ������ô�������5�Σ�����ֵΪ0
    }
    else
    {
        call_count = 0;
        return 1; // ������ô����ﵽ5�Σ�����ֵΪ1
    }
}
inline static int RX_GET_IO(int *result)
{

    static int call_count = 0;     // ��̬�������ڼ�¼���������õĴ���
    static int positive_count = 0; // ��̬�������ڼ�¼����Ϊ���Ĵ���
    static int first_value = 0;    // ��̬�������ڴ洢��һ�ζ�ȡ����ֵ

    call_count++; // ÿ�ε���ʱ���Ӽ���

    if (call_count == 1)
    { // ����ǵ�һ�ε��ã���ȡ���ŵ�ֵ���洢
        first_value = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10);
    }

    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) > 0)
    { // ������ŵ�ֵΪ��������positive_count
        positive_count++;
    }

    if (call_count < MAX_TIMER3_FILTER)
    { // ������ô�������5�Σ����ص�һ�ζ�ȡ����ֵ
        *result = 0;
        return first_value;
    }
    else
    { // ������ô����ﵽ5�Σ��ж�����Ϊ���Ĵ���
        *result = 1;
        if (positive_count >= 3)
        {
            call_count = 0;
            positive_count = 0;
            first_value = 0;
            return 1; // �������Ϊ���Ĵ������ڵ���3������1
        }
        else
        {
            call_count = 0;
            positive_count = 0;
            first_value = 0;
            return 0; // ���򷵻�0
        }
    }
}
volatile int l_RX_Flag = 0;                     // �����˲���ɱ�ʶ

void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
        if (sending)
        {
            volatile static uint8_t dataToSend;
            volatile static uint8_t bitIndex = 0;
            volatile int iFlag = 0; // �˲���ɱ�ʶ
            if (bitIndex == 0)
            {
                // ���Ϳ�ʼλ
                // GPIO_ResetBits(GPIOA, GPIO_Pin_9);
                iFlag = TX_SET_IO(0);
                if (iFlag == 1)
                {
                    {
                        dataToSend = sendBuffer[0];
                        sendIndex--;
                        bitIndex++;
                    }
                }
                else if (bitIndex <= l_nUartWordLength)
                {
                    // ��������λ
                    if (dataToSend & 0x01)
                    {
                        // GPIO_SetBits(GPIOA, GPIO_Pin_9);
                        iFlag = TX_SET_IO(1);
                    }
                    else
                    {
                        // GPIO_ResetBits(GPIOA, GPIO_Pin_9);
                        iFlag = TX_SET_IO(0);
                    }
                    if (iFlag == 1)
                    {
                        dataToSend >>= 1;
                        bitIndex++;
                    }
                }
            }
            else
            {
                // ����ֹͣλ
                // GPIO_SetBits(GPIOA, GPIO_Pin_9);
                iFlag = TX_SET_IO(1);
                if (iFlag == 1)
                {
                    bitIndex = 0;
                    if (sendIndex > 0)
                    {
                        // ��������Ҫ���ͣ�����������һ���ֽ�
                        TIM_SetCounter(TIM3, 0);
                    }
                    else
                    {
                        // ������ɣ����÷��ͱ�־
                        sending = 0;
                    }
                }
            }
        }

        if (receiving)
        {
            volatile static uint8_t receivedData = 0;
            volatile static uint8_t bitIndex = 0;
            volatile static uint8_t busIdelCounter = 0; // ���߿��м���
            static uint8_t flagFirst=0;
						static char buff[32];
#define MAX_IDEL_TIME (100)                             // ���߿���ʱ��
            if (bitIndex == 0)
            {
                if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10)== 0)
                {
                    volatile static uint8_t counter_RX = 0;
                    if(counter_RX++ > 3)           {    // ��⵽��ʼλ
                        counter_RX = 0;
                        bitIndex++;
                        busIdelCounter = 0;
                        receivedData >>= 1;
                    }
                }
                else
                {
                    if (busIdelCounter++ > MAX_IDEL_TIME)
                    {
                        busIdelCounter = MAX_IDEL_TIME;
                        if (UART_IO_RxCount > 0) // ���ջ�����������,�������ݿ���ʱ�䳬��MAX_IDEL_TIME
                        {
                            UART_IO_ReceiveState = 1;
                        }
                    }
                }
            }
            else if (bitIndex > 0 && bitIndex <= l_nUartWordLength)
            {
                // ��������λ
                int value=RX_GET_IO(&l_RX_Flag);
                if (l_RX_Flag == 1) {
                    receivedData = receivedData >>1; // �ѽ��յ�������λ�������
                    if (value == 1)
                    {
                        receivedData |= 0x80;
                    }
                    else
                    {
                        receivedData |= 0x00;
                    }
										buff[flagFirst++]=receivedData;
										bitIndex++;
                }  
            }
            else if (bitIndex == l_nUartWordLength + 1)
            {
                // ��⵽ֹͣλ
                // receiveBuffer[receiveIndex++] = receivedData;
                // �ѽ��յ����ֽڱ��棬�����ַ��1
                UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
								receivedData=0;
                bitIndex = 0;
            }
        }
        else
        {
            // ���û�����ڽ������ݣ���⿪ʼλ
            if (RX_GET_IO(&l_RX_Flag) == 0)
            {
                receiving = 1;
            }
        }

        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}
