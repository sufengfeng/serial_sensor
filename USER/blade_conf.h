/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BLADE_CONF_H
#define __BLADE_CONF_H

#include "stm32f10x.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#define BLADE_CLI // CLI命令行模块，位于blade/CLI
// #define BLADE_XMODEM										//Xmodem模块

#include "cli_api.h"
#define UART1_BUFFER_LEN 256

#endif /*__BLADE_CONF_H*/
