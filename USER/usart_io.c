#include "usart_io.h"
#include "stm32f10x.h"
#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "uart.h"
// 定义一些常量
// #define BAUD_RATE 9600
#define TIMER_PRESCALER 1
// #define TIMER_PERIOD ((72000000 / TIMER_PRESCALER) / BAUD_RATE)

// 发送缓冲区和接收缓冲区
volatile uint8_t sendBuffer[8];
volatile uint8_t sendIndex = 0;

// 发送和接收状态标志
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
    l_nUartWordLength = USART_WordLength; // 设置有效位
    // 开启定时器 3 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_Cmd(TIM3, DISABLE); // 关闭定时器，防止冲突
    int TIMER_PERIOD = (72000000 / BAUD_RATE);

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
    sendIndex = 0;
    UART_IO_RxCount = 0;
    UART_IO_ReceiveState = 0;
    // 启动定时器
    TIM_Cmd(TIM3, ENABLE);
}

void USART_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 开启 GPIOA 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // 配置 PA9 为推挽输出，用于模拟串口发送
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 外部上拉推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置 PA10 为浮空输入，用于模拟串口接收
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 外部上拉输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;        //外部浮空输入
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
            // 等待发送完成
        }
        Uart_SendByte(str[i]);
    }
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
                // 发送开始位
                GPIO_ResetBits(GPIOA, GPIO_Pin_9);
                dataToSend = sendBuffer[0];
                sendIndex--;
                bitIndex++;
            }
            else if (bitIndex <= l_nUartWordLength)
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
            volatile static uint8_t receivedData = 0;
            volatile static uint8_t bitIndex = 0;
            volatile static uint8_t busIdelCounter = 0; // 总线空闲计数
#define MAX_IDEL_TIME (100)                    // 总线空闲时间
            if (bitIndex == 0)
            {
                if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) == 0)
                {
                    // 检测到开始位
                    bitIndex++;
                    busIdelCounter = 0;
                }
                else
                {
                    if (busIdelCounter++ > MAX_IDEL_TIME)
                    {
                        busIdelCounter = MAX_IDEL_TIME;
                        if (UART_IO_RxCount > 0) // 接收缓冲区有数据,且有数据空闲时间超过MAX_IDEL_TIME
                        {
                            UART_IO_ReceiveState = 1;
                        }
                    }
                }
            }
            else if (bitIndex > 0 && bitIndex <= l_nUartWordLength)
            {
                // 接收数据位
                receivedData >>= 1;
                if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10))
                    receivedData |= 0x80;
                bitIndex++;
            }
            else if (bitIndex == l_nUartWordLength + 1)
            {
                // 检测到停止位
                // receiveBuffer[receiveIndex++] = receivedData;
                // 把接收到的字节保存，数组地址加1
                UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
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
