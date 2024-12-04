#include "usart_io.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_exti.h"
#include "uart.h"
// ����һЩ����
// #define BAUD_RATE 9600
#define TIMER_PRESCALER 1
// #define TIMER_PERIOD ((72000000 / TIMER_PRESCALER) / BAUD_RATE)
// ����У��ģʽö��
typedef enum
{
    NO_PARITY,  // ��У��
    ODD_PARITY, // ��У��
    EVEN_PARITY // żУ��
} ParityMode;

// ����ֹͣλģʽö��
typedef enum
{
    None_STOP_BIT, // ��
    ONE_STOP_BIT,  // 1λֹͣλ
    TWO_STOP_BITS  // 2λֹͣλ
} StopBitMode;

volatile static ParityMode currentParityMode = NO_PARITY;      // ȫ�ֱ����������趨��ǰУ��ģʽ
volatile static StopBitMode currentStopBitMode = ONE_STOP_BIT; // ȫ�ֱ����������趨��ǰֹͣλģʽ

// ���ͻ������ͽ��ջ�����
volatile uint8_t sendBuffer[8];
volatile uint8_t sendIndex = 0;

// ���ͺͽ���״̬��־
volatile uint8_t sending = 0;
volatile uint8_t receiving = 1;
extern uint8_t UART_IO_RxBuffer[128];
extern uint8_t UART_IO_RxCount;
extern uint8_t UART_IO_ReceiveState;

void USART_GPIO_Init(void);
void Uart_SendByte(uint8_t data);
uint8_t Uart_ReceiveByte(void);
void TIM3_IRQHandler(void);
#define MAX_TIMER3_FILTER (5) // ÿλ��Ҫ��ʱ���ж�������
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
    currentParityMode = USART_Parity;    // ����У��ģʽ
    currentStopBitMode = USART_StopBits; // ����ֹͣλģʽ

    l_nUartWordLength = USART_WordLength; // ������Чλ
    // ������ʱ�� 3 ʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_Cmd(TIM3, DISABLE); // �رն�ʱ������ֹ��ͻ
    // int TIMER_PERIOD = (72000000 / BAUD_RATE);
    int TIMER_PERIOD = (14400000 / BAUD_RATE); // int TIMER_PERIOD = (72000000 / BAUD_RATE)/MAX_TIMER3_FILTER;
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

    // ����GPIOA��AFIO��ʱ�ӣ�ʹ���ⲿ�ж���Ҫ����AFIOʱ�ӣ�
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    // ���� PA9 Ϊ�������������ģ�⴮�ڷ���
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // �ⲿ�����������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_SetBits(GPIOA, GPIO_Pin_9);
    GPIO_SetBits(GPIOA, GPIO_Pin_10);

#if DEBUG_SIMULATOR == 0
    // ���� PA10 Ϊ�������룬����ģ�⴮�ڽ���
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // �ڲ���������
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // �ⲿ��������
    GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif
    {
        EXTI_InitTypeDef EXTI_InitStructure;
        NVIC_InitTypeDef NVIC_InitStructure;
        // �����ⲿ�ж��ߣ�PA10��ӦEXTI10
        GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource10);
        EXTI_InitStructure.EXTI_Line = EXTI_Line10;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
        EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // ����Ϊ�����ش���
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        EXTI_Init(&EXTI_InitStructure);

        // ����NVIC��ʹ��EXTI10�ж�ͨ�������������ȼ������������Ϊ�ϵ����ȼ����ɸ���ʵ�����������
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
// ��ʱ��3�жϴ���������������
void SendTask(void)
{
    if (sending)
    {
        volatile static uint8_t dataToSend;
        volatile static uint8_t bitIndex = 0;
        volatile static uint8_t parityBit = 0;

        if (bitIndex == 0)
        {
            // ���Ϳ�ʼλ
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
            // ��������λ
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
                // ��У�飬ֱ�ӽ��뷢��ֹͣλ����
                // bitIndex++; // ע�͵���һ�У���Ϊ����Ҫ����bitIndex
                GPIO_SetBits(GPIOA, GPIO_Pin_9);
#if DEBUG_SIMULATOR
                GPIO_SetBits(GPIOA, GPIO_Pin_10);
#endif
                bitIndex = 0;
                parityBit = 0;
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
        else if (bitIndex == l_nUartWordLength + (currentStopBitMode == ONE_STOP_BIT ? 2 : 3))
        {
            // ����ֹͣλģʽ����ֹͣλ
            GPIO_SetBits(GPIOA, GPIO_Pin_9);
#if DEBUG_SIMULATOR
            GPIO_SetBits(GPIOA, GPIO_Pin_10);
#endif
            bitIndex = 0;
            parityBit = 0;
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
        else
        {
            // ���Ͷ����ֹͣλ������� 2 λֹͣλģʽ��
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
volatile static uint8_t bitIndex = 0; // ����λ����
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
                    bitIndex = 0;
                    receiving = 1; // ������ɣ�׼��������һ������
                }
            }
        }
        else if (bitIndex > 0 && bitIndex <= l_nUartWordLength)
        {
            // ��������λ
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
        else if (bitIndex == l_nUartWordLength + 1) // ����У��λ
        {
            if (currentParityMode != NO_PARITY)
            {
                receivedParityBit = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10);
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
#endif // ����LED��˸����
        static uint8_t sendCounter = 0;
        if (sendCounter++ > MAX_TIMER3_FILTER-2)
        {
            sendCounter = 0;
            SendTask(); // ���÷���������
            ReceiveTask(); // ���ý���������
        }
        //ReceiveTask(); // ���ý���������

        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}

// �ⲿ�жϷ�����������PA10���ŵ��ж��¼�
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line10) != RESET)
    {
        // �ж��Ƿ��ǽ���״̬�����reciver=0����bitIndex=0��������receiveΪ1
        if (receiving == 0 && bitIndex == 0)
        {
            receiving = 1;
        }

        // ����жϱ�־λ�������ظ������жϣ��ǳ���Ҫ�Ĳ�����
        EXTI_ClearITPendingBit(EXTI_Line10);
    }
}