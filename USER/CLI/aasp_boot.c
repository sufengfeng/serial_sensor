#include "blade_conf.h"
#ifdef BLADE_CLI
#include "aasp_shell.h"


extern Subcmd *CliCommandList;

uint8_t boot_flag = 0;

extern int Cli_Register(const char *str, const char *help, int (*func)(int, char **));


void uboot_cli_install(void)
{

}
#endif
