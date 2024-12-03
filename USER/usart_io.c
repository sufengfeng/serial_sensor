#include "usart_io.h"
#include "stm32f10x.h"
#include "stm32f10x.h"
#include "stm32f10x_usart.h"
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
typedef enum {
    ONE_STOP_BIT,    // 1位停止位
    TWO_STOP_BITS    // 2位停止位
} StopBitMode;

volatile static ParityMode currentParityMode = NO_PARITY;   // 全局变量，用于设定当前校验模式
volatile static StopBitMode currentStopBitMode = ONE_STOP_BIT;      // 全局变量，用于设定当前停止位模式

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

void Timer3_Init(int BAUD_RATE, int USART_WordLength,int USART_Parity,int USART_StopBits)
{
    if (BAUD_RATE < 4800 || BAUD_RATE > 115200)
    {
        BAUD_RATE = 9600;
    }
    if (USART_WordLength < 7 || USART_WordLength > 9)
    {
        USART_WordLength = 8;
    }
    currentParityMode = USART_Parity;    // 设置校验模式
    currentStopBitMode = USART_StopBits; // 设置停止位模式
    
    l_nUartWordLength = USART_WordLength; // 设置有效位
    // 开启定时器 3 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_Cmd(TIM3, DISABLE); // 关闭定时器，防止冲突
    int TIMER_PERIOD = (72000000 / BAUD_RATE);
    // int TIMER_PERIOD = (72000000 / BAUD_RATE)/MAX_TIMER3_FILTER;
    // int TIMER_PERIOD = (14400000 / BAUD_RATE);
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
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 外部上拉推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_SetBits(GPIOA, GPIO_Pin_9);
    GPIO_SetBits(GPIOA, GPIO_Pin_10);

    // 配置 PA10 为浮空输入，用于模拟串口接收
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 内部上拉输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 外部浮空输入
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

inline static int TX_SET_IO(int value)
{

    volatile static int call_count = 0; // 静态变量用于记录函数被调用的次数
    // 每次调用时增加计数
    call_count++;
    if (value == 1)
    { // 根据value设置A9引脚的电平
        GPIO_SetBits(GPIOA, GPIO_Pin_9);
    }
    else
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_9);
    }
    if (call_count < MAX_TIMER3_FILTER)
    {
        return 0; // 如果调用次数少于5次，返回值为0
    }
    else
    {
        call_count = 0;
        return 1; // 如果调用次数达到5次，返回值为1
    }
}
inline static int RX_GET_IO(int *result)
{

    static int call_count = 0;     // 静态变量用于记录函数被调用的次数
    static int positive_count = 0; // 静态变量用于记录引脚为正的次数
    static int first_value = 0;    // 静态变量用于存储第一次读取到的值

    call_count++; // 每次调用时增加计数

    if (call_count == 1)
    { // 如果是第一次调用，读取引脚的值并存储
        first_value = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10);
    }

    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) > 0)
    { // 如果引脚的值为正，增加positive_count
        positive_count++;
    }

    if (call_count < MAX_TIMER3_FILTER)
    { // 如果调用次数少于5次，返回第一次读取到的值
        *result = 0;
        return first_value;
    }
    else
    { // 如果调用次数达到5次，判断引脚为正的次数
        *result = 1;
        if (positive_count >= 3)
        {
            call_count = 0;
            positive_count = 0;
            first_value = 0;
            return 1; // 如果引脚为正的次数大于等于3，返回1
        }
        else
        {
            call_count = 0;
            positive_count = 0;
            first_value = 0;
            return 0; // 否则返回0
        }
    }
}
volatile int l_RX_Flag = 0; // 接收滤波完成标识
// 使用5次采样保证数据的稳定性
void TIM3_IRQHandler_Filter(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
        TriggerBoardLed();
        if (sending)
        {
            volatile static uint8_t dataToSend;
            volatile static uint8_t bitIndex = 0;
            volatile int iFlag = 0; // 滤波完成标识
            if (bitIndex == 0)
            {
                // 发送开始位
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
                    // 发送数据位
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
                // 发送停止位
                // GPIO_SetBits(GPIOA, GPIO_Pin_9);
                iFlag = TX_SET_IO(1);
                if (iFlag == 1)
                {
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
        }

        if (receiving)
        {
            volatile static uint8_t receivedData = 0;
            volatile static uint8_t bitIndex = 0;
            volatile static uint8_t busIdelCounter = 0; // 总线空闲计数
            static uint8_t flagFirst = 0;
            static char buff[32];
#define MAX_IDEL_TIME (100) // 总线空闲时间
            if (bitIndex == 0)
            {
                if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) == 0)
                {
                    volatile static uint8_t counter_RX = 0;
                    if (counter_RX++ > 3)
                    { // 检测到开始位
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
                int value = RX_GET_IO(&l_RX_Flag);
                if (l_RX_Flag == 1)
                {
                    receivedData = receivedData >> 1; // 把接收到的数据位存入变量
                    if (value == 1)
                    {
                        receivedData |= 0x80;
                    }
                    else
                    {
                        receivedData |= 0x00;
                    }
                    buff[flagFirst++] = receivedData;
                    bitIndex++;
                }
            }
            else if (bitIndex == l_nUartWordLength + 1)
            {
                // 检测到停止位
                // receiveBuffer[receiveIndex++] = receivedData;
                // 把接收到的字节保存，数组地址加1
                UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
                receivedData = 0;
                bitIndex = 0;
            }
        }
        else
        {
            // 如果没有正在接收数据，检测开始位
            if (RX_GET_IO(&l_RX_Flag) == 0)
            {
                receiving = 1;
            }
        }

        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}
// 配置A10随A9变化，测试接收逻辑
void TIM3_IRQHandler_test(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
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

        if (sending)
        {
            static uint8_t dataToSend;
            static uint8_t bitIndex = 0;

            if (bitIndex == 0)
            {
                // 发送开始位
                GPIO_ResetBits(GPIOA, GPIO_Pin_9);
                GPIO_ResetBits(GPIOA, GPIO_Pin_10);
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
                    GPIO_SetBits(GPIOA, GPIO_Pin_10);
                }
                else
                {
                    GPIO_ResetBits(GPIOA, GPIO_Pin_9);
                    GPIO_ResetBits(GPIOA, GPIO_Pin_10);
                }
                dataToSend >>= 1;
                bitIndex++;
            }
            else
            {
                // 发送停止位
                GPIO_SetBits(GPIOA, GPIO_Pin_9);
                GPIO_SetBits(GPIOA, GPIO_Pin_10);
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
            static uint8_t busIdelCounter = 0; // 总线空闲计数
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
                receivedData = 0;
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
//9600 8N 1
void TIM3_IRQHandler_8N1(void)
{
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
    if (TIM_GetITStatus(TIM3, TIM_IT_Update)!= RESET)
    {
        
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
            static uint8_t receivedData = 0;
            static uint8_t bitIndex = 0;
            static uint8_t busIdelCounter = 0;      //总线空闲计数
            #define MAX_IDEL_TIME  (100) //总线空闲时间
            if (bitIndex == 0)
            {
                if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) == 0)
                {
                    // 检测到开始位
                    bitIndex++;
                    busIdelCounter=0;
                }else{
                    if(busIdelCounter++>MAX_IDEL_TIME){  
                        busIdelCounter=MAX_IDEL_TIME;
                        if (UART_IO_RxCount>0)  //接收缓冲区有数据,且有数据空闲时间超过MAX_IDEL_TIME
                        {
                            UART_IO_ReceiveState=1;    
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
            else if (bitIndex == l_nUartWordLength+1)
            {
                // 检测到停止位
                // receiveBuffer[receiveIndex++] = receivedData;
                //把接收到的字节保存，数组地址加1
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
//适配奇偶校验
void TIM3_IRQHandler_EVEN(void)
{
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
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
        if (sending)
        {
            static uint8_t dataToSend;
            static uint8_t bitIndex = 0;
            static uint8_t parityBit = 0;

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
                if (currentParityMode == EVEN_PARITY)
                {
                    parityBit ^= (dataToSend & 0x01);
                }
                else if (currentParityMode == ODD_PARITY)
                {
                    parityBit ^= (dataToSend & 0x01);
                }
                bitIndex++;
            }
            else if (bitIndex == l_nUartWordLength + 1)
            {
                if (currentParityMode != NO_PARITY)
                {
                    if (currentParityMode == EVEN_PARITY)
                    {
                        if (parityBit)
                            GPIO_SetBits(GPIOA, GPIO_Pin_9);
                        else
                            GPIO_ResetBits(GPIOA, GPIO_Pin_9);
                    }
                    else if (currentParityMode == ODD_PARITY)
                    {
                        if (parityBit)
                            GPIO_ResetBits(GPIOA, GPIO_Pin_9);
                        else
                            GPIO_SetBits(GPIOA, GPIO_Pin_9);
                    }
                    bitIndex++;
                }
                else
                {
                    // 无校验，直接进入发送停止位流程
                    bitIndex++;
                }
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
            static uint8_t busIdelCounter = 0;    // 总线空闲计数
            static uint8_t receivedParityBit = 0; // 新增用于接收奇偶校验位
            static uint8_t parityCheck = 0;       // 校验位

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
                    }
                }
            }
            else if (bitIndex > 0 && bitIndex <= l_nUartWordLength)
            {
                // 接收数据位
                receivedData >>= 1;
                if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10))
                    receivedData |= 0x80;
                if (currentParityMode == EVEN_PARITY)
                {
                    parityCheck ^= (receivedData & 0x01);
                }
                else if (currentParityMode == ODD_PARITY)
                {
                    parityCheck ^= (receivedData & 0x01);
                }
                bitIndex++;
            }
            else if (bitIndex == l_nUartWordLength + 1)
            {
                if (currentParityMode != NO_PARITY)
                {
                    receivedParityBit = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10);
                    bitIndex++;
                }
                else
                {
                    // 无校验，直接进入保存接收数据流程
                    // 把接收到的字节保存，数组地址加1
                    UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
                    bitIndex = 0;
                }
            }
            else if (bitIndex == l_nUartWordLength + 2)
            {
                if (currentParityMode != NO_PARITY)
                {
                    if (currentParityMode == EVEN_PARITY)
                    {
                        if (parityCheck == receivedParityBit)
                        {
                            // 校验通过，把接收到的字节保存，数组地址加1
                            UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
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
                            // 校验通过，把接收到的字节保存，数组地址加1
                            UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
                        }
                        else
                        {
                            // 校验失败，可以在这里添加相应错误处理逻辑，比如丢弃数据等
                            LOG(LOG_CRIT, "\r\parityCheck  [%d][%d]\r\n", parityCheck, receivedParityBit);
                        }
                    }
                    bitIndex = 0;
                    receivedParityBit = 0;
                    parityCheck = 0;
                }
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

void TIM3_IRQHandler(void)
{
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
    if (TIM_GetITStatus(TIM3, TIM_IT_Update)!= RESET)
    {
        if (sending)
        {
            static uint8_t dataToSend;
            static uint8_t bitIndex = 0;
            static uint8_t parityBit = 0;

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
                if (currentParityMode == EVEN_PARITY)
                {
                    parityBit ^= (dataToSend & 0x01);
                }
                else if (currentParityMode == ODD_PARITY)
                {
                    parityBit ^= (dataToSend & 0x01);
                }
                bitIndex++;
            }
            else if (bitIndex == l_nUartWordLength + 1)
            {
                if (currentParityMode!= NO_PARITY)
                {
                    if (currentParityMode == EVEN_PARITY)
                    {
                        if (parityBit)
                            GPIO_SetBits(GPIOA, GPIO_Pin_9);
                        else
                            GPIO_ResetBits(GPIOA, GPIO_Pin_9);
                    }
                    else if (currentParityMode == ODD_PARITY)
                    {
                        if (parityBit)
                            GPIO_ResetBits(GPIOA, GPIO_Pin_9);
                        else
                            GPIO_SetBits(GPIOA, GPIO_Pin_9);
                    }
                    bitIndex++;
                }
                else
                {
                    // 无校验，直接进入发送停止位流程
                    bitIndex++;
                }
            }
            else if (bitIndex == l_nUartWordLength + (currentStopBitMode == ONE_STOP_BIT? 2 : 3))
            {
                // 根据停止位模式发送停止位
                GPIO_SetBits(GPIOA, GPIO_Pin_9);
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
                    bitIndex++;
                }
            }
        }

        if (receiving)
        {
            static uint8_t receivedData = 0;
            static uint8_t bitIndex = 0;
            static uint8_t busIdelCounter = 0;    // 总线空闲计数
            static uint8_t receivedParityBit = 0;    // 接收到的奇偶校验位
            static uint8_t parityCheck = 0;            // 奇偶校验位检查

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
                    }
                }
            }
            else if (bitIndex > 0 && bitIndex <= l_nUartWordLength)
            {
                // 接收数据位
                receivedData >>= 1;
                if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10))
                    receivedData |= 0x80;
                if (currentParityMode == EVEN_PARITY)
                {
                    parityCheck ^= (receivedData & 0x01);
                }
                else if (currentParityMode == ODD_PARITY)
                {
                    parityCheck ^= (receivedData & 0x01);
                }
                bitIndex++;
            }
            else if (bitIndex == l_nUartWordLength + 1)
            {
                if (currentParityMode!= NO_PARITY)
                {
                    receivedParityBit = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10);
                    bitIndex++;
                }
                else
                {
                    // 无校验，直接进入保存接收数据流程
                    // 把接收到的字节保存，数组地址加1
                    UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
                    bitIndex = 0;
                }
            }
            else if (bitIndex == l_nUartWordLength + (currentStopBitMode == ONE_STOP_BIT? 2 : 3))
            {
                if (currentParityMode!= NO_PARITY)
                {
                    if (currentParityMode == EVEN_PARITY)
                    {
                        if (parityCheck == receivedParityBit)
                        {
                            // 校验通过，把接收到的字节保存，数组地址加1
                            UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
                        }
                        else
                        {
                            // 校验失败，可以在这里添加相应错误处理逻辑，比如丢弃数据等
                            LOG(LOG_CRIT, "\r\parityCheck  [%d][%d]\r\n", parityCheck, receivedParityBit);
                        }
                    }
                    else if (currentParityMode == ODD_PARITY)
                    {
                        if (parityCheck!= receivedParityBit)
                        {
                            // 校验通过，把接接到的字节保存，数组地址加1
                            UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
                        }
                        else
                        {
                            // 校验失败，可以在这里添加相应错误处理逻辑，比如丢弃数据等
                            LOG(LOG_CRIT, "\r\parityCheck  [%d][%d]\r\n", parityCheck, receivedParityBit);
                        }
                    }
                    bitIndex = 0;
                    receivedParityBit = 0;
                    parityCheck = 0;
                }
                else
                {
                    // 无校验时保存接收数据
                    UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
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

        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}