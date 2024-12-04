#include "usart_io.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_exti.h"
#include "uart.h"
// 定义一些常量
// #define BAUD_RATE 9600
#define TIMER_PRESCALER 1
// #define TIMER_PERIOD ((72000000 / TIMER_PRESCALER) / BAUD_RATE)
// 定义校验模式枚举
typedef enum
{
    NO_PARITY,  // 无校验
    ODD_PARITY, // 奇校验
    EVEN_PARITY // 偶校验
} ParityMode;

// 定义停止位模式枚举
typedef enum
{
    None_STOP_BIT, // 无
    ONE_STOP_BIT,  // 1位停止位
    TWO_STOP_BITS  // 2位停止位
} StopBitMode;

volatile static ParityMode currentParityMode = NO_PARITY;      // 全局变量，用于设定当前校验模式
volatile static StopBitMode currentStopBitMode = ONE_STOP_BIT; // 全局变量，用于设定当前停止位模式

// 发送缓冲区和接收缓冲区
volatile uint8_t sendBuffer[8];
volatile uint8_t sendIndex = 0;

// 发送和接收状态标志
volatile uint8_t sending = 0;
volatile uint8_t receiving = 1;
extern uint8_t UART_IO_RxBuffer[128];
extern uint8_t UART_IO_RxCount;
extern uint8_t UART_IO_ReceiveState;

void USART_GPIO_Init(void);
void Uart_SendByte(uint8_t data);
uint8_t Uart_ReceiveByte(void);
void TIM3_IRQHandler(void);
#define MAX_TIMER3_FILTER (5) // 每位需要定时器中断最大次数
volatile static int l_nUartWordLength = 8;

void Timer3_Init(int BAUD_RATE, int USART_WordLength, int USART_Parity, int USART_StopBits)
{
    if (BAUD_RATE < 4800 || BAUD_RATE > 115200)
    {
        BAUD_RATE = 9600;
    }
    if (USART_WordLength < 7 || USART_WordLength > 8)
    {
        USART_WordLength = 8;
    }
    if (USART_Parity < 0 || USART_Parity > 2)
    {
        USART_Parity = 0;
    }
    if (USART_StopBits < 1 || USART_StopBits > 2)
    {
        USART_StopBits = 1;
    }
    currentParityMode = USART_Parity;    // 设置校验模式
    currentStopBitMode = USART_StopBits; // 设置停止位模式

    l_nUartWordLength = USART_WordLength; // 设置有效位
    // 开启定时器 3 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_Cmd(TIM3, DISABLE); // 关闭定时器，防止冲突
    // int TIMER_PERIOD = (72000000 / BAUD_RATE);
    int TIMER_PERIOD = (14400000 / BAUD_RATE); // int TIMER_PERIOD = (72000000 / BAUD_RATE)/MAX_TIMER3_FILTER;
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

    // 开启GPIOA和AFIO的时钟（使用外部中断需要开启AFIO时钟）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    // 配置 PA9 为推挽输出，用于模拟串口发送
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 外部上拉推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_SetBits(GPIOA, GPIO_Pin_9);
    GPIO_SetBits(GPIOA, GPIO_Pin_10);

#if DEBUG_SIMULATOR == 0
    // 配置 PA10 为浮空输入，用于模拟串口接收
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 内部上拉输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 外部浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif
    {
        EXTI_InitTypeDef EXTI_InitStructure;
        NVIC_InitTypeDef NVIC_InitStructure;
        // 配置外部中断线，PA10对应EXTI10
        GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource10);
        EXTI_InitStructure.EXTI_Line = EXTI_Line10;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
        EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // 设置为上升沿触发
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        EXTI_Init(&EXTI_InitStructure);

        // 配置NVIC，使能EXTI10中断通道，并设置优先级（这里简单设置为较低优先级，可根据实际需求调整）
        NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
    }
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
// 定时器3中断处理函数，发送任务
void SendTask(void)
{
    if (sending)
    {
        volatile static uint8_t dataToSend;
        volatile static uint8_t bitIndex = 0;
        volatile static uint8_t parityBit = 0;

        if (bitIndex == 0)
        {
            // 发送开始位
            GPIO_ResetBits(GPIOA, GPIO_Pin_9);
#if DEBUG_SIMULATOR
            GPIO_ResetBits(GPIOA, GPIO_Pin_10);
#endif
            dataToSend = sendBuffer[0];
            sendIndex--;
            bitIndex++;
        }
        else if (bitIndex <= l_nUartWordLength)
        {
            // 发送数据位
            if (dataToSend & 0x01)
            {
                GPIO_SetBits(GPIOA, GPIO_Pin_9);
#if DEBUG_SIMULATOR
                GPIO_SetBits(GPIOA, GPIO_Pin_10);
#endif
            }
            else
            {
                GPIO_ResetBits(GPIOA, GPIO_Pin_9);
#if DEBUG_SIMULATOR
                GPIO_ResetBits(GPIOA, GPIO_Pin_10);
#endif
            }
            if (currentParityMode == EVEN_PARITY)
            {
                parityBit ^= (dataToSend & 0x01);
            }
            else if (currentParityMode == ODD_PARITY)
            {
                parityBit ^= (dataToSend & 0x01);
            }
            dataToSend >>= 1;
            bitIndex++;
        }
        else if (bitIndex == l_nUartWordLength + 1)
        {
            if (currentParityMode != NO_PARITY)
            {
                if (currentParityMode == EVEN_PARITY)
                {
                    if (parityBit)
                    {
                        GPIO_SetBits(GPIOA, GPIO_Pin_9);
#if DEBUG_SIMULATOR
                        GPIO_SetBits(GPIOA, GPIO_Pin_10);
#endif
                    }
                    else
                    {
                        GPIO_ResetBits(GPIOA, GPIO_Pin_9);
#if DEBUG_SIMULATOR
                        GPIO_ResetBits(GPIOA, GPIO_Pin_10);
#endif
                    }
                }
                else if (currentParityMode == ODD_PARITY)
                {
                    if (parityBit)
                    {
                        GPIO_ResetBits(GPIOA, GPIO_Pin_9);
#if DEBUG_SIMULATOR
                        GPIO_ResetBits(GPIOA, GPIO_Pin_10);
#endif
                    }
                    else
                    {
                        GPIO_SetBits(GPIOA, GPIO_Pin_9);
#if DEBUG_SIMULATOR
                        GPIO_SetBits(GPIOA, GPIO_Pin_10);
#endif
                    }
                }
                bitIndex++;
            }
            else
            {
                // 无校验，直接进入发送停止位流程
                // bitIndex++; // 注释掉这一行，因为不需要增加bitIndex
                GPIO_SetBits(GPIOA, GPIO_Pin_9);
#if DEBUG_SIMULATOR
                GPIO_SetBits(GPIOA, GPIO_Pin_10);
#endif
                bitIndex = 0;
                parityBit = 0;
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
        else if (bitIndex == l_nUartWordLength + (currentStopBitMode == ONE_STOP_BIT ? 2 : 3))
        {
            // 根据停止位模式发送停止位
            GPIO_SetBits(GPIOA, GPIO_Pin_9);
#if DEBUG_SIMULATOR
            GPIO_SetBits(GPIOA, GPIO_Pin_10);
#endif
            bitIndex = 0;
            parityBit = 0;
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
        else
        {
            // 发送额外的停止位（如果是 2 位停止位模式）
            if (currentStopBitMode == TWO_STOP_BITS && bitIndex == l_nUartWordLength + 2)
            {
                GPIO_SetBits(GPIOA, GPIO_Pin_9);
#if DEBUG_SIMULATOR
                GPIO_SetBits(GPIOA, GPIO_Pin_10);
#endif
                bitIndex++;
            }
        }
    }
}
volatile static uint8_t bitIndex = 0; // 接收位计数
// 定时器3中断处理函数，接收任务
void ReceiveTask(void)
{
    if (receiving)
    {
        volatile static uint8_t receivedData = 0;
        // static uint8_t bitIndex = 0;
        volatile static uint8_t busIdelCounter = 0;    // 总线空闲计数
        volatile static uint8_t receivedParityBit = 0; // 接收到的奇偶校验位
        volatile static uint8_t parityCheck = 0;       // 奇偶校验位检查

#define MAX_IDEL_TIME (100) // 总线空闲时间
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
                    bitIndex = 0;
                    receiving = 1; // 接收完成，准备接收下一个数据
                }
            }
        }
        else if (bitIndex > 0 && bitIndex <= l_nUartWordLength)
        {
            // 接收数据位
            receivedData >>= 1;
            uint8_t tmpValue = 0;
            tmpValue = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10);
            if (tmpValue == 1)
                receivedData |= 0x80;
            if (currentParityMode == EVEN_PARITY)
            {
                parityCheck ^= (tmpValue & 0x01);
            }
            else if (currentParityMode == ODD_PARITY)
            {
                parityCheck ^= (tmpValue & 0x01);
            }
            bitIndex++;
        }
        else if (bitIndex == l_nUartWordLength + 1) // 有无校验位
        {
            if (currentParityMode != NO_PARITY)
            {
                receivedParityBit = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10);
                bitIndex++;
            }
            else
            {
                if (l_nUartWordLength == 7)
                { // 如果数据为7位，则需右移一位
                    receivedData >>= 1;
                }
                // 无校验，直接进入保存接收数据流程
                // 把接收到的字节保存，数组地址加1
                UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
                receivedData = 0;
                bitIndex = 0;
                receivedParityBit = 0;
                parityCheck = 0;
            }
        }
        else if (bitIndex == l_nUartWordLength + (currentStopBitMode == ONE_STOP_BIT ? 2 : 3)) // 有无停止位
        {
            if (currentParityMode != NO_PARITY)
            {
                if (currentParityMode == EVEN_PARITY)
                {
                    if (parityCheck == receivedParityBit)
                    {
                        if (l_nUartWordLength == 7)
                        { // 如果数据为7位，则需右移一位
                            receivedData >>= 1;
                        }
                        // 校验通过，把接收到的字节保存，数组地址加1
                        UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
                        receivedData = 0;
                    }
                    else
                    {
                        // 校验失败，可以在这里添加相应错误处理逻辑，比如丢弃数据等
                        LOG(LOG_CRIT, "\r\parityCheck  [%d][%d]\r\n", parityCheck, receivedParityBit);
                    }
                }
                else if (currentParityMode == ODD_PARITY)
                {
                    if (parityCheck != receivedParityBit)
                    {
                        if (l_nUartWordLength == 7)
                        { // 如果数据为7位，则需右移一位
                            receivedData >>= 1;
                        }
                        // 校验通过，把接接到的字节保存，数组地址加1
                        UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
                        receivedData = 0;
                    }
                    else
                    {
                        // 校验失败，可以在这里添加相应错误处理逻辑，比如丢弃数据等
                        LOG(LOG_CRIT, "\r\parityCheck  [%d][%d][%d]\r\n", parityCheck, receivedParityBit, receivedData);
                    }
                }
                bitIndex = 0;
                receivedParityBit = 0;
                parityCheck = 0;
            }
            else
            {
                if (l_nUartWordLength == 7)
                { // 如果数据为7位，则需右移一位
                    receivedData >>= 1;
                }
                // 无校验时保存接收数据
                UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
                receivedData = 0;
                bitIndex = 0;
            }
        }
        else if (currentStopBitMode == TWO_STOP_BITS && bitIndex == l_nUartWordLength + 2)
        {
            // 等待第 2 位停止位，无实质操作，仅位计数增加
            bitIndex++;
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
}
void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
#if DEBUG_SIMULATOR
        TriggerBoardLed();
#endif // 调用LED闪烁函数
        static uint8_t sendCounter = 0;
        if (sendCounter++ > MAX_TIMER3_FILTER-2)
        {
            sendCounter = 0;
            SendTask(); // 调用发送任务函数
            ReceiveTask(); // 调用接收任务函数
        }
        //ReceiveTask(); // 调用接收任务函数

        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}

// 外部中断服务函数，处理PA10引脚的中断事件
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line10) != RESET)
    {
        // 判断是否是接收状态，如果reciver=0，且bitIndex=0，则设置receive为1
        if (receiving == 0 && bitIndex == 0)
        {
            receiving = 1;
        }

        // 清除中断标志位，避免重复进入中断（非常重要的操作）
        EXTI_ClearITPendingBit(EXTI_Line10);
    }
}