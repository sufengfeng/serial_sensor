#ifndef __UART__H_
#define __UART__H_
#include "stm32f10x.h"
#include "stm32f10x_usart.h"

void USART1_Config(void);
void USART2_Config(void);
void USART1_IRQHandler(void);
void USART2_IRQHandler(void);
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


#define LOG_LEVLE  (LOG_INFO)


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

#endif
