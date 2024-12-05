#include "blade_conf.h"
#include "stdlib.h"
#include "time.h"
#ifdef CMSIS_OS
#include "cmsis_os.h"
#endif

int32_t gpio_do(int argc,char *argv[])
{
    void * gpioPort = NULL;
    int16_t gpioPin = 0;
    int32_t val = 0;

    if(argc<3)
    {
        return 0;
    }

    if((strncmp(argv[1],"pa",2) ==0)||(strncmp(argv[1],"PA",2) ==0))
    {
        gpioPort = GPIOA;
    }
    else if((strncmp(argv[1],"pb",2) ==0)||(strncmp(argv[1],"PB",2) ==0))
    {
        gpioPort = GPIOB;
    }
    else if((strncmp(argv[1],"pc",2) ==0)||(strncmp(argv[1],"PC",2) ==0))
    {
        gpioPort = GPIOC;
    }
    else if((strncmp(argv[1],"pd",2) ==0)||(strncmp(argv[1],"PD",2) ==0))
    {
        gpioPort = GPIOD;
    }
    else if((strncmp(argv[1],"pe",2) ==0)||(strncmp(argv[1],"PE",2) ==0))
    {
        gpioPort = GPIOE;
    }
    else if((strncmp(argv[1],"pf",2) ==0)||(strncmp(argv[1],"PF",2) ==0))
    {
        gpioPort = GPIOF;
    }
    #ifdef GPIOG
    else if((strncmp(argv[1],"pg",2) ==0)||(strncmp(argv[1],"PG",2) ==0))
    {
        gpioPort = GPIOG;
    }
    #endif
    else
    {
        gpioPort = GPIOA;
    }

//    sscanf(argv[1],"%2s%d",buf,&val);
    val = atoi((char *)(argv[1]+2));
    printf("gpio  %d \r\n",val);

    if((val >0 )&&(val <= 16))
    {
        gpioPin = 1<<val;
    }
    else
    {
        return 0;
    }

    if(strncmp(argv[2],"on",2) ==0)
    {
        //HAL_GPIO_WritePin(gpioPort,gpioPin,GPIO_PIN_SET);
			GPIO_SetBits(gpioPort,gpioPin);
    }
    else if(strncmp(argv[2],"off",2) ==0)
    {
        //HAL_GPIO_WritePin(gpioPort,gpioPin,GPIO_PIN_RESET);
			GPIO_ResetBits(gpioPort,gpioPin);
    }
    else if(strncmp(argv[2],"init",1)==0)
    {
#ifndef USE_STDPERIPH_DRIVER    
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;

        GPIO_InitStruct.Pin = gpioPin;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(gpioPort, &GPIO_InitStruct);
#endif        
    }
	
	return 0;
}

#ifdef BLADE_DB
int do_input_dump(int argc,char *argv[])
{
    robot_input_dump();
    return 0;
}

int do_status_dump(int argc,char *argv[])
{
    robot_status_dump();
    return 0;
}
#endif
#ifdef CMSIS_OS
uint8_t debugBuf[1024];
int do_taskShow(int argc,char *argv[])
{    
    printf("%-16s%-16s%-13s%-10s%-5s%-10s%-8s\r\n",
                        "taskName",
                        "tcbxHandle",
                        "pxStackBase",
                        "state",
                        "pri",
                        "stackHigh",
                        "taskNo");
    osThreadList(debugBuf);   
    printf("%s\r\n",debugBuf);
   // vTaskGetRunTimeStats(debugBuf);
    //printf("%s\r\n",debugBuf);
    return 0;
}
#endif

#ifdef BLADE_CFG
int do_cfgShow(int argc,char *argv[])
{
    ini_dump();
    return 0;
}

#ifdef BLADE_CFG_MEM
int do_memShow(int argc,char *argv[])
{
    dictMemShow();
    return 0;
}
#endif
#endif

#ifdef BLADE_CLI_51TEST
__weak void SetMPRobotFlag(bool enable) 
{

}

__weak void SetMPStartFlag(bool enable) 
{

}

int do_51test(int argc,char *argv[])
{
	if(argc ==1)
	{
		SetMPStartFlag(true);
	}
	else if(argc == 2)
	{
		if((strncmp(argv[1],"pcba",4) ==0)||(strncmp(argv[1],"PCBA",4) ==0))//板级测试
		{
			SetMPStartFlag(true);
		}
		else if((strncmp(argv[1],"z",1) ==0)||(strncmp(argv[1],"z",1) ==0))//整机/模块测试
		{
			SetMPRobotFlag(true);
		}
	}
	return 0;
}
#endif

__weak void delay(uint32_t t)
{
	while (t--);
}

// int do_delay_test(int argc, char *argv[])
// {
// 	uint32_t delays = 0;
// 	uint32_t tick1 = 0;
// 	uint32_t tick2 = 0;

// 	delays = strtoul(argv[1],NULL,10);

// 	tick1 = HAL_GetTick();
// 	delay(delays);
// 	tick2 = HAL_GetTick();

// 	printf("delay %u cost tick %u (ms)\r\n",delays,(tick2>=tick1)?(tick2-tick1):(tick2+0xffffffff-tick1));	
        
//     return 0;
// }

#ifdef BLADE_BATTERY
extern void DBGPrintBatteryData(void);
int do_batteryShow(int argc,char *argv[])
{
	DBGPrintBatteryData();
	return 0;
}
#endif

#ifdef INCLUDE_SYNC_TIME
extern void set_print_time(uint64_t time);
#endif

__weak uint64_t GetGlobalSystemTime(void)
{
    return 0;
}

void print_systemtime(void)
{
    struct tm *nowtime;
	uint64_t localTime = GetGlobalSystemTime()/1000;
	nowtime = localtime((time_t*)&localTime);


    printf("%04d-%02d-%02d %02d:%02d:%02d.%03u (%llu sec)\r\n\r\n",nowtime->tm_year+1900
											,nowtime->tm_mon+1
											,nowtime->tm_mday
                                            ,nowtime->tm_hour
											,nowtime->tm_min
											,nowtime->tm_sec
                                            ,(uint32_t)(GetGlobalSystemTime()%(1000))
                                            ,localTime);
	
}
int do_date_show(int argc,char *argv[])
{
    if(argc ==1 )
    {
        print_systemtime();
    }
   #ifdef INCLUDE_SYNC_TIME 
    else if(argc == 3)
    {
        uint64_t time = 0;
        time = strtoull(argv[2],NULL,10);
        set_print_time(time*1000);
    }
    #endif
    return 0;
} 

void common_cli_install(void)
{
#ifdef BLADE_DB
    Cli_Register("inputs","inputs dump?",do_input_dump);
    Cli_Register("status","status dump?",do_status_dump);
#endif
    Cli_Register("gpio","gpio pXY on|off",gpio_do);
#ifdef CMSIS_OS    
    //Cli_Register("top","taskShow",do_taskShow);
#endif    
#ifdef BLADE_CFG_MEM    
    //调试的时候打开，其它时候不用打开；
    //Cli_Register("memshow","memory Show",do_memShow);    
#endif
#ifdef BLADE_CFG
    Cli_Register("cfgshow","cfgshow",do_cfgShow);
#endif
#ifdef BLADE_BATTERY
    Cli_Register("battery","battery info",do_batteryShow);
#endif

#ifdef BLADE_CLI_51TEST
    Hide_Cli_Register("51test","51test pcba|z",do_51test);
#endif

#ifdef BLADE_BSP_EEPROM
    /*DEBUG*/
    nameplate_cli_install();
#endif	

#ifdef BLADE_OTA
    ota_cli_install();
#endif

#ifdef BLADE_RTL8367  
    sw_cli_install();
#endif
  
#ifdef SMI_DEBUG
    smi_cli_install();
#endif
//    Cli_Register("delay","delay 0-4294967295",do_delay_test);

    Cli_Register("date","date [set_print 0-4294967295]",do_date_show);

#ifdef BLADE_ACCURACY_TEST
    extern void accuracy_test_cli_install(void);
    accuracy_test_cli_install();
#endif
}
