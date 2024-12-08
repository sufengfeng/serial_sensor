#include "usart_io.h"
#include "uart.h"

// ����һЩ����
// #define BAUD_RATE 9600
#define TIMER_PRESCALER 1
// #define TIMER_PERIOD ((72000000 / TIMER_PRESCALER) / BAUD_RATE)

// ���ͻ������ͽ��ջ�����
// volatile uint8_t sendBuffer[8];
// volatile uint8_t sendIndex = 0;

// ���ͺͽ���״̬��־
volatile uint8_t sending = 0;
volatile uint8_t receiving = 1;
extern uint8_t UART_IO_RxBuffer[128];
extern uint8_t UART_IO_RxCount;
extern uint8_t UART_IO_ReceiveState;

uint8_t recvStat = COM_STOP_BIT; // ����״̬��

// #define MAX_TIMER3_FILTER (5) // ÿλ��Ҫ��ʱ���ж�������


// ȫ�ֶ���Ĺ̶��ڴ��ַ��ѭ������ʵ��
CircularQueue fixedQueue;

// ��ʼ��ѭ������
void InitCircularQueue()
{
    fixedQueue.front = 0;
    fixedQueue.rear = 0;
    fixedQueue.size = 0;
}

// ��Ӳ���
void enqueue(char element)
{
    if (fixedQueue.size == QUEUE_SIZE)
    {
        // ��������
        printf("Queue is full. Cannot enqueue element.\n");
        return;
    }
    fixedQueue.data[fixedQueue.rear] = element;
    fixedQueue.rear = (fixedQueue.rear + 1) % QUEUE_SIZE; // ѭ���ƶ���βָ��
    fixedQueue.size++;
}

// ���Ӳ���������������������ͨ�������������ݣ�������Ϊ���򷵻�0
int dequeue(char *element)
{
    if (fixedQueue.size == 0)
    {
        return 0;
    }
    *element = fixedQueue.data[fixedQueue.front];
    fixedQueue.front = (fixedQueue.front + 1) % QUEUE_SIZE; // ѭ���ƶ���ͷָ��
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

    // ���һЩԪ��
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
    InitCircularQueue();                              // ��ʼ��ѭ������

    // ������ʱ�� 3 ʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_Cmd(TIM3, DISABLE); // �رն�ʱ������ֹ��ͻ
    // int TIMER_PERIOD = (72000000 / 115200);
    int TIMER_PERIOD = 625;
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
    // sendIndex = 0;
    UART_IO_RxCount = 0;
    UART_IO_ReceiveState = 0;
    // ������ʱ��
    TIM_Cmd(TIM3, ENABLE);
}

void USART_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // ����GPIOA��AFIO��ʱ�ӣ�ʹ���ⲿ�ж���Ҫ����AFIOʱ�ӣ�
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /*	����֤*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    COM_DATA_HIGH();
    COM_RX_HIGH();

    // ����TXD
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
#if DEBUG_SIMULATOR == 0
    // ����RXD
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
#else
    // ���� PB7 Ϊ�������룬����ģ�⴮�ڽ���
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // �ⲿ��������
    GPIO_Init(GPIOB, &GPIO_InitStructure);
#endif

    {
        EXTI_InitTypeDef EXTI_InitStructure;
        NVIC_InitTypeDef NVIC_InitStructure;
        // �����ⲿ�ж��ߣ�PB7��ӦEXTI7
        GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource7);
        EXTI_InitStructure.EXTI_Line = EXTI_Line7;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
        EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // ����Ϊ�����ش���
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        EXTI_Init(&EXTI_InitStructure);

        // ����NVIC��ʹ��EXTI7�ж�ͨ�������������ȼ������������Ϊ�ϵ����ȼ����ɸ���ʵ�����������
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
// ��ʱ��3�жϴ���������������
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
            // ���Ϳ�ʼλ
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
                // У��ʧ�ܣ����������������Ӧ�������߼������綪�����ݵ�
                LOG(LOG_CRIT, "\r\nCannot get element\r\n", );
                sending = 0;
            }
        }
        else if (bitIndex <= 8)
        {
            // ��������λ
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
                // ��У�飬ֱ�ӽ��뷢��ֹͣλ����
                // bitIndex++; // ע�͵���һ�У���Ϊ����Ҫ����bitIndex
                COM_DATA_HIGH();
#if DEBUG_SIMULATOR
                COM_RX_HIGH();
#endif
                bitIndex = 0;
                if (GetDequeueSize() > 0)
                {
                    // ��������Ҫ���ͣ�����������һ���ֽ�
                    ;
                    // TIM_SetCounter(TIM3, 0);
                }
                else
                {
                    // ������ɣ����÷��ͱ�־
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
// volatile static uint8_t bitIndex = 0; // ����λ����
uint8_t recvData;

// ��ʱ��3�жϴ���������������
void ReceiveTask(void)
{
    if (receiving)
    {
        static uint8_t receivedData = 0;
        static uint8_t bitIndex = 0;
        static uint8_t busIdelCounter = 0; // ���߿��м���
#define MAX_IDEL_TIME (100)                // ���߿���ʱ��
        if (bitIndex == 0)
        {
            if (COM_RX_STAT == 0)
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
        else if (bitIndex > 0 && bitIndex <= 8)
        {
            // ��������λ
            receivedData >>= 1;
            if (COM_RX_STAT)
                receivedData |= 0x80;
            bitIndex++;
        }
        else if (bitIndex >= 9)
        {
            // ��⵽ֹͣλ
            UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
            bitIndex = 0;
            receivedData= 0;
        }
    }
    // else
    // {
    //     // ���û�����ڽ������ݣ���⿪ʼλ
    //     if (COM_RX_STAT == 0)
    //     {
    //         receiving = 1;
    //     }
    // }
}

/*
// ��ʱ��3�жϴ���������������
void ReceiveTask(void)
{
    if (receiving)
    {
        volatile static uint8_t receivedData = 0;
        // static uint8_t bitIndex = 0;
        volatile static uint8_t busIdelCounter = 0;    // ���߿��м���
        volatile static uint8_t receivedParityBit = 0; // ���յ�����żУ��λ
        volatile static uint8_t parityCheck = 0;       // ��żУ��λ���

#define MAX_IDEL_TIME (100) // ���߿���ʱ��
        if (bitIndex == 0)
        {
            if (GPIO_ReadInputDataBit(COM_RX_PORT, COM_RX_PIN) == 0)
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
                    bitIndex = 0;
                    receiving = 1; // ������ɣ�׼��������һ������
            receivedData = 0;
                        receivedParityBit = 0;
                        parityCheck = 0;
                }
            }
        }
        else if (bitIndex > 0 && bitIndex <= l_nUartWordLength)	// ��������λ
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
        else if (bitIndex == l_nUartWordLength + 1) // ����У��λ
        {
            if (currentParityMode != NO_PARITY)
            {
                receivedParityBit = GPIO_ReadInputDataBit(COM_RX_PORT, COM_RX_PIN);
                bitIndex++;
            }
            else
            {
                if (l_nUartWordLength == 7)
                { // �������Ϊ7λ����������һλ
                    receivedData >>= 1;
                }
                // ��У�飬ֱ�ӽ��뱣�������������
                // �ѽ��յ����ֽڱ��棬�����ַ��1
                UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
                receivedData = 0;
                bitIndex = 0;
                receivedParityBit = 0;
                parityCheck = 0;
            }
        }
        else if (bitIndex == l_nUartWordLength + (currentStopBitMode == ONE_STOP_BIT ? 2 : 3)) // ����ֹͣλ
        {
            if (currentParityMode != NO_PARITY)
            {
                if (currentParityMode == EVEN_PARITY)
                {
                    if (parityCheck == receivedParityBit)
                    {
                        if (l_nUartWordLength == 7)
                        { // �������Ϊ7λ����������һλ
                            receivedData >>= 1;
                        }
                        // У��ͨ�����ѽ��յ����ֽڱ��棬�����ַ��1
                        UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
                        receivedData = 0;
                    }
                    else
                    {
                        // У��ʧ�ܣ����������������Ӧ�������߼������綪�����ݵ�
                        LOG(LOG_CRIT, "\r\parityCheck  [%d][%d]\r\n", parityCheck, receivedParityBit);
                    }
                }
                else if (currentParityMode == ODD_PARITY)
                {
                    if (parityCheck != receivedParityBit)
                    {
                        if (l_nUartWordLength == 7)
                        { // �������Ϊ7λ����������һλ
                            receivedData >>= 1;
                        }
                        // У��ͨ�����ѽӽӵ����ֽڱ��棬�����ַ��1
                        UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
                        receivedData = 0;
                    }
                    else
                    {
                        // У��ʧ�ܣ����������������Ӧ�������߼������綪�����ݵ�
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
                { // �������Ϊ7λ����������һλ
                    receivedData >>= 1;
                }
                // ��У��ʱ�����������
                UART_IO_RxBuffer[UART_IO_RxCount++] = receivedData;
                receivedData = 0;
                bitIndex = 0;
            }
        }
        else if (currentStopBitMode == TWO_STOP_BITS && bitIndex == l_nUartWordLength + 2)
        {
            // �ȴ��� 2 λֹͣλ����ʵ�ʲ�������λ��������
            bitIndex++;
        }
    }
    else
    {
        // ���û�����ڽ������ݣ���⿪ʼλ
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
        TriggerBoardLed(); // ����LED��˸����
#endif
        SendTask();    // ���÷���������
        ReceiveTask(); // ���ý���������
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}
// void Delay(u32 t)

// {
//     while (t--)
//         ;
// }

// ���������һ��ȷ������ʱ��������ʱ���ж�

// ��Ҫ���ͣ�������ʱ���ж�,����������

// �ⲿ�жϷ�����������PB7���ŵ��ж��¼�
void EXTI9_5_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line7) != RESET)
    {
        if (receiving == 0 && recvStat == COM_STOP_BIT) // �ж��Ƿ��ǽ���״̬�����reciver=0����bitIndex=0��������receiveΪ1
        {
            receiving = 1;
            // recvStat = COM_START_BIT;
            if (sending == 0)
            { // �����δ����״̬���������ʱ��
                // TIM_Cmd(TIM3, DISABLE);     // �رն�ʱ��
                TIM_SetCounter(TIM3, 300); // ���м�λ�ò���
                // TIM_Cmd(TIM3, ENABLE);      // �򿪶�ʱ������������
            }
        }
        // ����жϱ�־λ�������ظ������жϣ��ǳ���Ҫ�Ĳ�����
        EXTI_ClearITPendingBit(EXTI_Line7);
    }
}
