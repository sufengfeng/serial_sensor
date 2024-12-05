#include "blade_conf.h"
#ifdef BLADE_OTA
extern int Cli_Register(const char *str, const char *help, int (*func)(int, char **));

#ifndef BLADE_OTA_NOSTORE_HEAD
//ota-update 
int do_update_main(int argc, char *argv[])
{
	#ifdef BLADE_CAN_CLI
	return ota_can_cli_update();
	#elif defined BLADE_XMODEM
	return ota_xmodem_update();
	#else
	return 0;
	#endif
}
#endif


void ota_cli_install(void)
{
#ifndef BLADE_OTA_NOSTORE_HEAD
#if defined(BLADE_XMODEM)||defined(BLADE_CAN_CLI)
	Cli_Register("upgrade","upgrade app program or config",do_update_main);
#endif
#endif	
	Cli_Register("version","show robot version",do_show_version);

}
#endif
