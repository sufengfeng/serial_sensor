#include "usart_io.h"
#include "stm32f10x.h"
#include "stm32f10x.h"
#include "stm32f10x_usart.h"

// 定义一些常量
// #define BAUD_RATE 9600
#define TIMER_PRESCALER 72
// #define TIMER_PERIOD ((72000000 / TIMER_PRESCALER) / BAUD_RATE)

// 发送缓冲区和接收缓冲区
uint8_t sendBuffer[64];
uint8_t receiveBuffer[64];
uint8_t sendIndex = 0;
uint8_t receiveIndex = 0;

// 发送和接收状态标志
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
   

    // 假设要发送的数据
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
        // 处理接收的数据
        if (receiveIndex > 0)
        {
            // 在这里可以对接收的数据进行处理
            // for (int i = 0; i < receiveIndex; i++)
            // {
            //     // 例如，将接收到的数据再次发送出去
            //     sendBuffer[sendIndex++] = receiveBuffer[i];
            // }
            printf("Received data: %d\n",Uart_ReceiveByte());
            receiveIndex = 0;
        }
    }
}
void Timer3_Init(int BAUD_RATE)
{
    // 开启定时器 3 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_Cmd(TIM3, DISABLE);
    int TIMER_PERIOD=(1000000/ BAUD_RATE);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period = TIMER_PERIOD - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = TIMER_PRESCALER - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    // 开启定时器中断
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    // 配置 NVIC，使能定时器中断
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

    // 开启 GPIOA 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // 配置 PA9 为推挽输出，用于模拟串口发送
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置 PA10 为浮空输入，用于模拟串口接收
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}


void Uart_SendByte(uint8_t data)
{
    // 将数据放入发送缓冲区
    sendBuffer[sendIndex++] = data;
    // 如果当前没有正在发送的数据，则启动发送过程
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
        // 将接收缓冲区中的数据向前移动
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
                // 发送开始位
                GPIO_ResetBits(GPIOA, GPIO_Pin_9);
                dataToSend = sendBuffer[0];
                sendIndex--;
                bitIndex++;
            }
            else if (bitIndex <= 8)
            {
                // 发送数据位
                if (dataToSend & 0x01)
                    GPIO_SetBits(GPIOA, GPIO_Pin_9);
                else
                    GPIO_ResetBits(GPIOA, GPIO_Pin_9);
                dataToSend >>= 1;
                bitIndex++;
            }
            else
            {
                // 发送停止位
                GPIO_SetBits(GPIOA, GPIO_Pin_9);
                bitIndex = 0;
                if (sendIndex > 0)
                {
                    // 还有数据要发送，继续发送下一个字节
                    TIM_SetCounter(TIM3, 0);
                }
                else
                {
                    // 发送完成，重置发送标志
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
                    // 检测到开始位
                    bitIndex++;
                }
            }
            else if (bitIndex > 0 && bitIndex <= 8)
            {
                // 接收数据位
                receivedData >>= 1;
                if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10))
                    receivedData |= 0x80;
                bitIndex++;
            }
            else if (bitIndex == 9)
            {
                // 检测到停止位
                receiveBuffer[receiveIndex++] = receivedData;
                bitIndex = 0;
            }
        }
        else
        {
            // 如果没有正在接收数据，检测开始位
            if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) == 0)
            {
                receiving = 1;
            }
        }

        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}