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
    int TIMER_PERIOD = (72000000 / BAUD_RATE);

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
    // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // �ⲿ��������
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;        //�ⲿ��������
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}
void Uart_SendByteStr(uint8_t *str, int len)
{
    for (int i = 0; i < len; i++)
    {   
        uint32_t counterTimeout = 0;
        while (sending)
        {
            if(counterTimeout++>1000000) {
                LOG(LOG_CRIT, "\r\nUart_SendByteStr  timeout\r\n"      );
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

void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
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

        if (sending)
        {
            volatile static uint8_t dataToSend;
            volatile static uint8_t bitIndex = 0;

            if (bitIndex == 0)
            {
                // ���Ϳ�ʼλ
                GPIO_ResetBits(GPIOA, GPIO_Pin_9);
                dataToSend = sendBuffer[0];
                sendIndex--;
                bitIndex++;
            }
            else if (bitIndex <= l_nUartWordLength)
            {
                // ��������λ
                if (dataToSend & 0x01)
                    GPIO_SetBits(GPIOA, GPIO_Pin_9);
                else
                    GPIO_ResetBits(GPIOA, GPIO_Pin_9);
                dataToSend >>= 1;
                bitIndex++;
            }
            else
            {
                // ����ֹͣλ
                GPIO_SetBits(GPIOA, GPIO_Pin_9);
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

        if (receiving)
        {
            volatile static uint8_t receivedData = 0;
            volatile static uint8_t bitIndex = 0;
            volatile static uint8_t busIdelCounter = 0; // ���߿��м���
#define MAX_IDEL_TIME (100)                    // ���߿���ʱ��
            if (bitIndex == 0)
            {
                if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) == 0)
                {
                    // ��⵽��ʼλ
                    bitIndex++;
                    busIdelCounter = 0;
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
                receivedData >>= 1;
                if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10))
                    receivedData |= 0x80;
                bitIndex++;
            }
            else if (bitIndex == l_nUartWordLength + 1)
            {
                // ��⵽ֹͣλ
                // receiveBuffer[receiveIndex++] = receivedData;
                // �ѽ��յ����ֽڱ��棬�����ַ��1
                UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
                bitIndex = 0;
            }
        }
        else
        {
            // ���û�����ڽ������ݣ���⿪ʼλ
            if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) == 0)
            {
                receiving = 1;
            }
        }

        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}
