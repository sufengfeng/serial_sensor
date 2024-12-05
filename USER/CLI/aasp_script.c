#include "blade_conf.h"
#ifdef BLADE_CLI
#include "aasp_shell.h"

extern int do_help(int argc, char *argv[]);
extern void uboot_cli_install(void);
int CliNumOfCommands = 0;
Subcmd * CliCommandList = NULL;

/*!@brief   Register a command to CLI.
 * @example Cli_Register("help","show help text",&builtin_help);
 *
 * @param   name      Command name
 * @param   prompt    Command prompt text
 * @param   func      Pointer to function to run when the command is called.
 *
 * @retval  index    The index of the command is inserted in the command list.
 * @retval  -1       Command register fail.
 */
int Cli_Register(const char *str, const char *help, int (*func)(int, char **))
{
	int i = 0;
    if ((str == NULL) || (help == NULL) || func == NULL)
    {
        return -1;
    }

    for (i = 0; i < CLI_COMMAND_SIZE; i++)
    {
		if((CliCommandList[i].str != NULL)&&(strcmp(CliCommandList[i].str,str) ==0))
		{
				//repeat register
				return -1;
		}
	}
		
    for (i = 0; i < CLI_COMMAND_SIZE; i++)
    {
        // Find a empty slot to save the command.
        if ((CliCommandList[i].str == NULL) && (CliCommandList[i].fun == NULL))
        {
            CliCommandList[i].str = (char *)str;
            CliCommandList[i].help = (char *)help;
            CliCommandList[i].fun = func;
			CliCommandList[i].bHide = false;

            CliNumOfCommands++;
            return i;
        }
    }

    return -1;
}
/*藏起来不让别人看�?*/
int Hide_Cli_Register(const char *str, const char *help, int (*func)(int, char **))
{
	int i =0;

	i = Cli_Register(str,help,func);

	if(i>=0)
	{
		CliCommandList[i].bHide = true;
	}
	
	return i;
}
#if 0
int Cli_Unregister(const char *str)
{
    if ((str == NULL) || (str[0] == 0))
    {
        return -1;
    }

    for (int i = 0; i < CLI_COMMAND_SIZE; i++)
    {
        // Delete the command
        if (strcmp(CliCommandList[i].str, str) == 0)
        {
            CliCommandList[i].str = NULL;
            CliCommandList[i].help = NULL;
            CliCommandList[i].fun = NULL;
            CliNumOfCommands--;
            return i;
        }
    }
    return -1;
}
#endif
void *cli_malloc(size_t size)
{

    void *ptr = NULL;
    while (ptr == NULL)
    {
        ptr = (void *)malloc(size);
    }
    memset(ptr, 0, size);
    return ptr;
}

void cli_free(void *ptr)
{
    free(ptr);
}


int do_memory_modify(int argc, char *argv[])
{
	uint32_t addr = 0;
	uint32_t newValue = 0;
	
	
	if(argc < 3)
	{
		printf("cmd error");
		return -1;
	}
	
	addr = strtoul(argv[1],NULL,16);
	newValue = strtoul(argv[2],NULL,16);
	
	*(uint32_t *)addr = newValue;
	
	printf("0x%8x : %08x \r\n",addr,*(uint32_t *)addr);
	return 0;
}

int do_memory_display(int argc, char *argv[])
{
	uint32_t len = 0;
	uint32_t addr = 0;
	uint32_t i = 0;
	uint8_t widthShow=4;

	if(argc < 2)
	{
		printf("cmd error");
		return -1;
	}
	
	addr = strtoul(argv[1],NULL,16);
	if(argc <3)
	{
		len = 1;	
	}
	else
	{
		len = strtoul(argv[2],NULL,10);
	}
	
	if(argc == 4)
	{
		widthShow = atoi(argv[3]);
		if((widthShow!=4)
			&&(widthShow!=2)
			&&(widthShow !=1))
		{
			printf("display width should be 1,2 or 4 \r\n");
			return 0;
		}
	}

	for(i=0;i<len;i++)
	{
		if(i%(16/widthShow) == 0)
		{
			printf("0x%08x :",addr+i*widthShow);
		}

		if(widthShow ==4)
		{
			printf("%08x ",*(uint32_t *)(addr+i*4));
		}
		else if(widthShow ==2)
		{
			printf("%04x ",*(uint16_t *)(addr+i*2));
		}
		else
		{
			printf("%02x ",*(uint8_t *)(addr+i));
		}

		if((i%(16/widthShow) == (16/widthShow-1))||(i == len-1))
		{
			printf("\r\n");
		}
	}
	return 0;
}

//reboot
int do_reboot(int argc, char *argv[])
{
	delay(100);
	__disable_irq();
	NVIC_SystemReset();
	return 0;
}

int Cli_Init(void)
{
	CliCommandList = cli_malloc(sizeof(Subcmd) * CLI_COMMAND_SIZE);
	#ifdef BOOTLOADER
	Cli_Register("h","Help about help?",do_help);
	#else
	Cli_Register("help","Help about help?",do_help);
	#endif

	#ifdef BOOTLOADER
	uboot_cli_install();
	Cli_Register("m","mm 0x0-0xFFFFFFFF 0xXXXXXXXX",do_memory_modify);
	Cli_Register("d","md 0x0-0xFFFFFFFF <1-100> [WIDTH]",do_memory_display);
	#else
	Cli_Register("mm","mm 0x0-0xFFFFFFFF 0xXXXXXXXX",do_memory_modify);
	Cli_Register("md","md 0x0-0xFFFFFFFF <1-100> [WIDTH]",do_memory_display);
	Cli_Register("reboot","reboot",do_reboot);
	#endif
	return 0;
}
#endif
