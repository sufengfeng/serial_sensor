#include "usart_io.h"
#include "uart.h"

// 定义一些常量
// #define BAUD_RATE 9600
#define TIMER_PRESCALER 1
// #define TIMER_PERIOD ((72000000 / TIMER_PRESCALER) / BAUD_RATE)

// 发送缓冲区和接收缓冲区
// volatile uint8_t sendBuffer[8];
// volatile uint8_t sendIndex = 0;

// 发送和接收状态标志
volatile uint8_t sending = 0;
volatile uint8_t receiving = 1;
extern uint8_t UART_IO_RxBuffer[128];
extern uint8_t UART_IO_RxCount;
extern uint8_t UART_IO_ReceiveState;

uint8_t recvStat = COM_STOP_BIT; // 定义状态机

// #define MAX_TIMER3_FILTER (5) // 每位需要定时器中断最大次数


// 全局定义的固定内存地址的循环队列实例
CircularQueue fixedQueue;

// 初始化循环队列
void InitCircularQueue()
{
    fixedQueue.front = 0;
    fixedQueue.rear = 0;
    fixedQueue.size = 0;
}

// 入队操作
void enqueue(char element)
{
    if (fixedQueue.size == QUEUE_SIZE)
    {
        // 队列已满
        printf("Queue is full. Cannot enqueue element.\n");
        return;
    }
    fixedQueue.data[fixedQueue.rear] = element;
    fixedQueue.rear = (fixedQueue.rear + 1) % QUEUE_SIZE; // 循环移动队尾指针
    fixedQueue.size++;
}

// 出队操作，若队列中有数据则通过参数返回数据，若队列为空则返回0
int dequeue(char *element)
{
    if (fixedQueue.size == 0)
    {
        return 0;
    }
    *element = fixedQueue.data[fixedQueue.front];
    fixedQueue.front = (fixedQueue.front + 1) % QUEUE_SIZE; // 循环移动队头指针
    fixedQueue.size--;
    return 1;
}
int GetDequeueSize(void)
{
    return fixedQueue.size;
}
int maintest()
{
    InitCircularQueue();

    // 入队一些元素
    char elements[] = "abcdefg";
    for (int i = 0; i < strlen(elements); i++)
    {
        enqueue(elements[i]);
    }

    char dequeuedElement;
    while (dequeue(&dequeuedElement))
    {
        printf("%c ", dequeuedElement);
    }
    printf("\n");

    return 0;
}
void Timer3_Init(void)
{
    InitCircularQueue();                              // 初始化循环队列

    // 开启定时器 3 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_Cmd(TIM3, DISABLE); // 关闭定时器，防止冲突
    // int TIMER_PERIOD = (72000000 / 115200);
    int TIMER_PERIOD = 625;
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
    // sendIndex = 0;
    UART_IO_RxCount = 0;
    UART_IO_ReceiveState = 0;
    // 启动定时器
    TIM_Cmd(TIM3, ENABLE);
}

void USART_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 开启GPIOA和AFIO的时钟（使用外部中断需要开启AFIO时钟）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /*	待验证*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    COM_DATA_HIGH();
    COM_RX_HIGH();

    // 配置TXD
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
#if DEBUG_SIMULATOR == 0
    // 配置RXD
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
#else
    // 配置 PB7 为浮空输入，用于模拟串口接收
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 外部浮空输入
    GPIO_Init(GPIOB, &GPIO_InitStructure);
#endif

    {
        EXTI_InitTypeDef EXTI_InitStructure;
        NVIC_InitTypeDef NVIC_InitStructure;
        // 配置外部中断线，PB7对应EXTI7
        GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource7);
        EXTI_InitStructure.EXTI_Line = EXTI_Line7;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
        EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // 设置为上升沿触发
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        EXTI_Init(&EXTI_InitStructure);

        // 配置NVIC，使能EXTI7中断通道，并设置优先级（这里简单设置为较低优先级，可根据实际需求调整）
        NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
    }
}

void Uart_IO_SendByteStr(char *str, int len)
{
    for (int i = 0; i < len; i++)
    {
        Uart_IO_SendByte(str[i]);
    }
}

void Uart_IO_SendByte(uint8_t data)
{
    enqueue(data);
    sending = 1;
}
// 定时器3中断处理函数，发送任务
void SendTask(void)
{
    // if (GetDequeueSize() > 0)
    // {
    //     sending = 1;
    // }
    if (sending)
    {
        volatile static uint8_t dataToSend;
        volatile static uint8_t bitIndex = 0;
        if (bitIndex == 0)
        {
            // 发送开始位
            COM_DATA_LOW();
#if DEBUG_SIMULATOR
            COM_RX_LOW();
#endif
            char dequeuedElement;
            if (dequeue(&dequeuedElement))
            {
                dataToSend = dequeuedElement;
                // sendIndex--;
                bitIndex++;
            }
            else
            {
                // 校验失败，可以在这里添加相应错误处理逻辑，比如丢弃数据等
                LOG(LOG_CRIT, "\r\nCannot get element\r\n", );
                sending = 0;
            }
        }
        else if (bitIndex <= 8)
        {
            // 发送数据位
            if (dataToSend & 0x01)
            {
                COM_DATA_HIGH();
#if DEBUG_SIMULATOR
                COM_RX_HIGH();
#endif
            }
            else
            {
                COM_DATA_LOW();
#if DEBUG_SIMULATOR
                COM_RX_LOW();
#endif
            }
            dataToSend >>= 1;
            bitIndex++;
        }
        else if (bitIndex == 9)
        {

            {
                // 无校验，直接进入发送停止位流程
                // bitIndex++; // 注释掉这一行，因为不需要增加bitIndex
                COM_DATA_HIGH();
#if DEBUG_SIMULATOR
                COM_RX_HIGH();
#endif
                bitIndex = 0;
                if (GetDequeueSize() > 0)
                {
                    // 还有数据要发送，继续发送下一个字节
                    ;
                    // TIM_SetCounter(TIM3, 0);
                }
                else
                {
                    // 发送完成，重置发送标志
                    sending = 0;
                }
            }
        }
        else
        {
            LOG(LOG_CRIT, "\r\nUart_SendByteStr  timeout\r\n");
        }
    }
}
// volatile static uint8_t bitIndex = 0; // 接收位计数
uint8_t recvData;

// 定时器3中断处理函数，接收任务
void ReceiveTask(void)
{
    if (receiving)
    {
        static uint8_t receivedData = 0;
        static uint8_t bitIndex = 0;
        static uint8_t busIdelCounter = 0; // 总线空闲计数
#define MAX_IDEL_TIME (100)                // 总线空闲时间
        if (bitIndex == 0)
        {
            if (COM_RX_STAT == 0)
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
        else if (bitIndex > 0 && bitIndex <= 8)
        {
            // 接收数据位
            receivedData >>= 1;
            if (COM_RX_STAT)
                receivedData |= 0x80;
            bitIndex++;
        }
        else if (bitIndex >= 9)
        {
            // 检测到停止位
            UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
            bitIndex = 0;
            receivedData= 0;
        }
    }
    // else
    // {
    //     // 如果没有正在接收数据，检测开始位
    //     if (COM_RX_STAT == 0)
    //     {
    //         receiving = 1;
    //     }
    // }
}

/*
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
            if (GPIO_ReadInputDataBit(COM_RX_PORT, COM_RX_PIN) == 0)
            {
                // 检测到起始位
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
            receivedData = 0;
                        receivedParityBit = 0;
                        parityCheck = 0;
                }
            }
        }
        else if (bitIndex > 0 && bitIndex <= l_nUartWordLength)	// 接收数据位
        {

            receivedData >>= 1;
            uint8_t tmpValue = 0;
            tmpValue = GPIO_ReadInputDataBit(COM_RX_PORT, COM_RX_PIN);
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
                receivedParityBit = GPIO_ReadInputDataBit(COM_RX_PORT, COM_RX_PIN);
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
        if (GPIO_ReadInputDataBit(COM_RX_PORT, COM_RX_PIN) == 0)
        {
            receiving = 1;
        }
    }
}
*/
void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
#if DEBUG_SIMULATOR
        TriggerBoardLed(); // 调用LED闪烁函数
#endif
        SendTask();    // 调用发送任务函数
        ReceiveTask(); // 调用接收任务函数
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}
// void Delay(u32 t)

// {
//     while (t--)
//         ;
// }

// 接受完进行一个确定的延时，开启定时器中断

// 需要发送，则开启定时器中断,并发送数据

// 外部中断服务函数，处理PB7引脚的中断事件
void EXTI9_5_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line7) != RESET)
    {
        if (receiving == 0 && recvStat == COM_STOP_BIT) // 判断是否是接收状态，如果reciver=0，且bitIndex=0，则设置receive为1
        {
            receiving = 1;
            // recvStat = COM_START_BIT;
            if (sending == 0)
            { // 如果是未发送状态，则调整定时器
                // TIM_Cmd(TIM3, DISABLE);     // 关闭定时器
                TIM_SetCounter(TIM3, 300); // 在中间位置采样
                // TIM_Cmd(TIM3, ENABLE);      // 打开定时器，接收数据
            }
        }
        // 清除中断标志位，避免重复进入中断（非常重要的操作）
        EXTI_ClearITPendingBit(EXTI_Line7);
    }
}
