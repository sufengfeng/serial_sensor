#include "usart_io.h"
#include "stm32f10x.h"
#include "stm32f10x.h"
#include "stm32f10x_usart.h"

// ����һЩ����
// #define BAUD_RATE 9600
#define TIMER_PRESCALER 72
// #define TIMER_PERIOD ((72000000 / TIMER_PRESCALER) / BAUD_RATE)

// ���ͻ������ͽ��ջ�����
uint8_t sendBuffer[64];
uint8_t receiveBuffer[64];
uint8_t sendIndex = 0;
uint8_t receiveIndex = 0;

// ���ͺͽ���״̬��־
volatile uint8_t sending = 0;
volatile uint8_t receiving = 0;

// void Timer3_Init(void);
void Timer3_Init(int BAUD_RATE);
void USART_GPIO_Init(void);
void Uart_SendByte(uint8_t data);
uint8_t Uart_ReceiveByte(void);
void TIM3_IRQHandler(void);
uint8_t GetUartIOCounter(void){
    return receiveIndex;
}

int main1(void)
{
	USART_GPIO_Init();
    Timer3_Init(9600);
   

    // ����Ҫ���͵�����
    sendBuffer[0] = 'H';
    sendBuffer[1] = 'e';
    sendBuffer[2] = 'l';
    sendBuffer[3] = 'l';
    sendBuffer[4] = 'o';
    sendBuffer[5] = '\n';
    sendIndex = 6;
    //Uart_SendByte(0x04);
    while (1)
    {
        // ������յ�����
        if (receiveIndex > 0)
        {
            // ��������ԶԽ��յ����ݽ��д���
            // for (int i = 0; i < receiveIndex; i++)
            // {
            //     // ���磬�����յ��������ٴη��ͳ�ȥ
            //     sendBuffer[sendIndex++] = receiveBuffer[i];
            // }
            printf("Received data: %d\n",Uart_ReceiveByte());
            receiveIndex = 0;
        }
    }
}
void Timer3_Init(int BAUD_RATE)
{
    // ������ʱ�� 3 ʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_Cmd(TIM3, DISABLE);
    int TIMER_PERIOD=(1000000/ BAUD_RATE);

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

    TIM_Cmd(TIM3, ENABLE);
}

void USART_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // ���� GPIOA ʱ��
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // ���� PA9 Ϊ�������������ģ�⴮�ڷ���
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // ���� PA10 Ϊ�������룬����ģ�⴮�ڽ���
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
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

uint8_t Uart_ReceiveByte(void)
{
    if (receiveIndex > 0)
    {
        uint8_t data = receiveBuffer[0];
        // �����ջ������е�������ǰ�ƶ�
        for (int i = 0; i < receiveIndex - 1; i++)
        {
            receiveBuffer[i] = receiveBuffer[i + 1];
        }
        receiveIndex--;
        return data;
    }
    return 0;
}


void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update)!= RESET)
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
            static uint8_t dataToSend;
            static uint8_t bitIndex = 0;

            if (bitIndex == 0)
            {
                // ���Ϳ�ʼλ
                GPIO_ResetBits(GPIOA, GPIO_Pin_9);
                dataToSend = sendBuffer[0];
                sendIndex--;
                bitIndex++;
            }
            else if (bitIndex <= 8)
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
            static uint8_t receivedData = 0;
            static uint8_t bitIndex = 0;

            if (bitIndex == 0)
            {
                if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) == 0)
                {
                    // ��⵽��ʼλ
                    bitIndex++;
                }
            }
            else if (bitIndex > 0 && bitIndex <= 8)
            {
                // ��������λ
                receivedData >>= 1;
                if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10))
                    receivedData |= 0x80;
                bitIndex++;
            }
            else if (bitIndex == 9)
            {
                // ��⵽ֹͣλ
                receiveBuffer[receiveIndex++] = receivedData;
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