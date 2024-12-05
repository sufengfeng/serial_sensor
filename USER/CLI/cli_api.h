#ifndef _INCLUDE_CLI_API_H
#define _INCLUDE_CLI_API_H


#ifdef BLADE_CAN_CLI
#define CAN_CLI_QURY_FUNCID 0x160
#define CAN_CLI_TRANS_FUNCID 0x170

extern int32_t drv_can_send( uint8_t *data,uint32_t stdId, uint8_t len);
extern void print_out_complete(void);
#endif

int Cli_Register(const char *str, const char *help, int (*func)(int, char **));
int Hide_Cli_Register(const char *str, const char *help, int (*func)(int, char **));
extern int Cli_Init(void);
extern void aasp_shell_nos(void);
extern void aasp_init(void);
extern void ota_cli_install(void);
void plc_cli_install(void);
int uputc(char ch);
void delay(uint32_t t);
#endif
