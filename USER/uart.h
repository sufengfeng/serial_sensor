#ifndef __UART__H_
#define __UART__H_
#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "stdio.h"
void USART1_Config(int BAUD_RATE, int USART_WordLength, int USART_Parity, int USART_StopBits);
void USART2_Config(void);
void USART3_Config(void);
void USART1_IRQHandler(void);
void USART2_IRQHandler(void);
void USART1_SendByte(uint16_t Data);
void USART2_SendByte(uint16_t Data);
void USART3_SendByte(uint16_t Data);
void USART_SendByte(USART_TypeDef* USARTx,uint16_t Data);

#ifndef LOG_EMERG
#define	LOG_EMERG	0	/* system is unusable */
#endif

#ifndef LOG_ALERT
#define	LOG_ALERT	1	/* action must be taken immediately */
#endif

#ifndef LOG_CRIT
#define	LOG_CRIT	2	/* critical conditions */
#endif

#ifndef LOG_ERR
#define	LOG_ERR		3	/* error conditions */
#endif

#ifndef LOG_WARNING
#define	LOG_WARNING	4	/* warning conditions */
#endif

#ifndef LOG_NOTICE
#define	LOG_NOTICE	5	/* normal but significant condition */
#endif

#ifndef LOG_INFO
#define	LOG_INFO	6	/* informational */
#endif

#ifndef LOG_DEBUG
#define	LOG_DEBUG	7	/* debug-level messages */
#endif

int GetLogLevel(void);
#define LOG_LEVLE  (GetLogLevel())

#define log_ printf
#define LOG(flag, fmt, args...)\
	do{\
		if (flag>LOG_LEVLE)\
			continue;\
		if(flag <= LOG_ERR){\
			log_("[ERR]:%s <L%d> : " fmt"\r\n",__FUNCTION__, __LINE__, ##args);\
		}else if(flag <= LOG_WARNING){\
			log_("[WARNING]:%s <L%d> : " fmt "\r\n",__FUNCTION__, __LINE__, ##args);\
		}else if(flag <= LOG_NOTICE){\
			log_("[NOTICE]:%s <L%d> : " fmt "\r\n",__FUNCTION__, __LINE__, ##args);\
		}else if(flag <= LOG_INFO){\
			log_("[INFO]:%s <L%d> : " fmt "\r\n", __FUNCTION__, __LINE__, ##args);\
		}else {\
			log_("[DEBUG]:%s <L%d> : " fmt "\r\n",__FUNCTION__, __LINE__, ##args);\
		}\
	}while(0)

int USART1_SendStr(char *str,uint8_t len);
void Uart_SendByteStr(char *str, int len);
int USART2_SendStr(uint8_t *str,uint8_t len);
void Set485SendMode(void);
void Set485ReceiveMode(void);
#endif
