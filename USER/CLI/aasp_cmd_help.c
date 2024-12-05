#include "blade_conf.h"
#ifdef BLADE_CLI
#include "aasp_shell.h"

extern Subcmd *CliCommandList;

int do_help(int argc, char *argv[])
{
	int i = 0;
	for (i = 0; i < CLI_COMMAND_SIZE; i++)
	{
		if ((CliCommandList[i].str != NULL) 
			&& (CliCommandList[i].fun != NULL)
			&&(!CliCommandList[i].bHide))
		{
			#ifdef BOOTLOADER
			iprintf("%s: %s \r\n", CliCommandList[i].str,CliCommandList[i].help);
			#else
			iprintf("%-20s: %s \r\n", CliCommandList[i].str,CliCommandList[i].help);
			#endif
		}
	}

    return 0;
}
#endif



