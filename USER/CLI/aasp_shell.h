#ifndef AASP_SHELL_H
#define AASP_SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define UART_CUR_NUM 0

#define iprintf printf
//#define iprintf SelfCliPrintf
#define siprintf sprintf

#define SUCCESS	0
#define ERROR	-1
#define bool char
#ifndef size_t
//#define size_t unsigned int
typedef unsigned int size_t;
#endif

//Request Attribute of Parameter
enum Request_Require{ optional,	required } ;

//Message Type
enum Message_Type{
	message, //default
	warning, //warning
	error, //error
	yesno //Yes or No for ASK
} ;	

//Auto Conversion of Input Type
typedef enum {
	OPTION_INPUT = 0, 	//need argument
	OPTION_GROUP_INPUT, 	//need argument
	OPTION_GROUP_SELECT,  	//need argument
	OPTION_SELECT,    	//no argument
	OPTION_FUNCTION,   	//no argument
	OPTION_RESERVED=0x7ffffff
} Option_Type;

//The Request Parameter Descriptor
typedef struct Request_Parameter
{
	const char *name;		//This field is the name of the option. It is a string.
	Option_Type otype;
	//char require;			//This field says whether the option takes an argument. 
	int require; // by lizg
	void *type;		//input, select, group, function
	const char *info;		//help message
	//char isProcessed;
	int isProcessed; //by lizg
} Param;

//Item of Group Type
typedef struct Group_Item
{
	const char *name; //Item name
	//char value; //Item's value
	int value; //Item's value, by lizg FPGA board char type will error, 
} Item;

//Type of Group
typedef struct Group_Type
{
	const char *name; //Group Name
	Item *items; //Group Items List
	char *result; //Which Items Value
} Group;

//Auto Conversion of Input Type
typedef enum {
	toString = 0, toInt, toLong, toFloat, toDouble,toReserved=0x7fffffff
} Input_Conv;

//Type of Input
typedef struct Input_Type
{
	//enum Input_Conv type; //Input Conversion Type
	Input_Conv type; //Input Conversion Type
	unsigned int length; // Max Length
	void *result;	// Target Buffer Pointer
} Input;

//Type of Input Array
typedef struct InputArray_Type
{
	//enum Input_Conv type; //Input Conversion Type
	Input_Conv type; //Input Conversion Type
	unsigned int length; // Max Length
	void *result;	// Target Buffer Pointer
	unsigned int number; //Number of Elements
} InputArray;

//Type of Select Type
typedef struct Select_Type
{
//	unsigned char isSelected; //Default set Selected if 1.
	unsigned char *result; //Selected:1 else 0
} Select;

typedef int (*option_fun)(void);
//Type of Funciton
typedef struct Function_Type
{
	//void *ptr; //Function Callback Pointer
	option_fun ptr; //Function Callback Pointer
	int *status; //Running Status
} Func;

//Type of Specific
typedef int (*key_fun)(void);
typedef struct Key_Type
{
	char key;//Key which assign
	char dumy[3];//padding 3 bytes
//	void *ptr; //Function Callback Pointer
	key_fun ptr; //Function Callback Pointer
} Key;


/*!
Wait for new command.(Create a application level shell).
\return The Status (success or failuire).
*/

typedef int subcmd_fun(int argc, char *argv[]);
typedef struct{
    char    *str;
    char    *help;
    subcmd_fun *fun;
	bool 	bHide;
}Subcmd;


/* -- Messenger -- */
/*!
Send a general message to User.
	\param msg	Message String.
\return The Status (success or failuire).
 */

/* --  Reporter  -- */

// Defines for debug verbosity level
extern int AASP_DebugLevel; 
#define DBGSTR_PREFIX "AASP: " 

#define DBGLVL_OFF              0       // if AASP_DebugLevel set to this, there is NO debug output 
#define DBGLVL_MINIMUM			1		// minimum verbosity	
#define DBGLVL_DEFAULT			2		// default verbosity level if no registry override
#define DBGLVL_MEDIUM			3		// medium verbosity
#define DBGLVL_HIGH             4       // highest 'safe' level (without severely affecting timing )
#define DBGLVL_MAXIMUM			5		// maximum level, may be dangerous

//Enter a function
#define ENTER()  \
		AASP_PrintCond(DBGLVL_HIGH,!ATE_MODE,("enter [%s]->\r\n",MOD_NAME)); \
		AASP_PrintCond(DBGLVL_HIGH,ATE_MODE,("enter [%s]->\r\n",MOD_NAME));


#define RETURN(RetVal) \
do{\
		AASP_PrintCond(DBGLVL_HIGH,ATE_MODE,("ret [%s]\r\n",MOD_NAME));\
	    return (RetVal);\
}while(0)	    

#define AASP_PrintCond( ilev, cond, _x_) \
        if( AASP_DebugLevel && ( ilev <= AASP_DebugLevel ) && ( cond )) { \
			iprintf _x_ ; \
	}

#define AASP_Print( ilev, _x_)  AASP_PrintCond( ilev, true, _x_ )

//Force Pass a Test
#define PASS(Name) \
do {\
	AASP_PrintCond(DBGLVL_HIGH,!ATE_MODE,("[%s]%s():pass!\r\n",MOD_NAME,Name)); \
	AASP_PrintCond(DBGLVL_HIGH,ATE_MODE,("<pass c=\"%s\">%s</pass>\r\n",MOD_NAME,Name)); \
}while(0)	

//Fore Fail a Test
#define FAIL(Name) \
do {\
	AASP_PrintCond(DBGLVL_HIGH,!ATE_MODE,("[%s]%s():fail!\r\n",MOD_NAME,Name)); \
	AASP_PrintCond(DBGLVL_HIGH,ATE_MODE,("<pass c=\"%s\">%s</pass>\r\n",MOD_NAME,Name)); \
}while(0)	

//Pass a test by true
#define OK(Result, Name)	\
	if(Result==0){			\
		AASP_PrintCond(DBGLVL_HIGH,ATE_MODE,("<pass c=\"%s\" t=\"ok\">%s</pass>\r\n",MOD_NAME,Name));	\
		AASP_PrintCond(DBGLVL_HIGH,!ATE_MODE,("[%s]%s:pass\r\n",MOD_NAME,Name));						\
	}else{																								\
		AASP_PrintCond(DBGLVL_HIGH,ATE_MODE,("<fail c=\"%s\" t=\"ok\">%s</fail>\r\n",MOD_NAME,Name));	\
		AASP_PrintCond(DBGLVL_HIGH,!ATE_MODE,("[%s]%s:fail\r\n",MOD_NAME,Name));						\
	}

//Pass a test by equal
#define EQ(Got, Expected, Name)	\
	if(Got==Expected){			\
		AASP_PrintCond(DBGLVL_HIGH,ATE_MODE,("<pass c=\"%s\" t=\"eq\">%s</pass>\r\n",MOD_NAME,Name));	\
		AASP_PrintCond(DBGLVL_HIGH,!ATE_MODE,("[%s]%s:pass\r\n",MOD_NAME,Name));						\
	}else{																								\
		AASP_PrintCond(DBGLVL_HIGH,ATE_MODE,("<fail c=\"%s\" t=\"eq\">%s</fail>\r\n",MOD_NAME,Name));	\
		AASP_PrintCond(DBGLVL_HIGH,!ATE_MODE,("[%s]%s:fail\r\n",MOD_NAME,Name));						\
	}

//Pass a test by not equal
#define NE(Got, DontExpected, Name)	\
	if(Got!=DontExpected){			\
		AASP_PrintCond(DBGLVL_HIGH,ATE_MODE,("<pass c=\"%s\" t=\"ne\">%s</pass>\r\n",MOD_NAME,Name));	\
		AASP_PrintCond(DBGLVL_HIGH,!ATE_MODE,("[%s]%s:pass\r\n",MOD_NAME,Name));						\
	}else{																								\
		AASP_PrintCond(DBGLVL_HIGH,ATE_MODE,("<fail c=\"%s\" t=\"ne\">%s</fail>\r\n",MOD_NAME,Name));	\
		AASP_PrintCond(DBGLVL_HIGH,!ATE_MODE,("[%s]%s:fail\r\n",MOD_NAME,Name));						\
	}

//Pass a test by conditional
#define CMP(This, Type, That, Name)	\
	if(This Type That){			\
		AASP_PrintCond(DBGLVL_HIGH,ATE_MODE,("<pass c=\"%s\" t=\"cmp\">%s</pass>\r\n",MOD_NAME,Name));	\
		AASP_PrintCond(DBGLVL_HIGH,!ATE_MODE,("[%s]%s:pass\r\n",MOD_NAME,Name));						\
	}else{																								\
		AASP_PrintCond(DBGLVL_HIGH,ATE_MODE,("<fail c=\"%s\" t=\"cmp\">%s</fail>\r\n",MOD_NAME,Name));	\
		AASP_PrintCond(DBGLVL_HIGH,!ATE_MODE,("[%s]%s:fail\r\n",MOD_NAME,Name));						\
	}

//Force Skip a test and tell me why?
#define SKIP(WHY) \
do {\
		AASP_PrintCond(DBGLVL_HIGH,ATE_MODE,("<skip c=\"%s\">%s</skip>\r\n",MOD_NAME,WHY));	\
		AASP_PrintCond(DBGLVL_HIGH,!ATE_MODE,("[%s]skip:%s\r\n",MOD_NAME,WHY));\
}while(0)		

//Mark a place need todo a test and tell me why?
#define TODO(WHY) \
do {\
		AASP_PrintCond(DBGLVL_HIGH,ATE_MODE,("<todo c=\"%s\">%s</todo>\r\n",MOD_NAME,WHY));	\
		AASP_PrintCond(DBGLVL_HIGH,!ATE_MODE,("[%s]todo:%s\r\n",MOD_NAME,WHY));\
}while(0)		

//Trap the test make stop.
#define TRAP() \
do {\
}while(0)	
	


#ifndef false
#define false 0
#endif

#ifndef true
#define true (!false)
#endif


extern int ATE_MODE;

#define MAX_ARGV 5
#define MAX_SCRIPT_LINE 128

extern int EXIT_FROM_CMDWAIT;

void aasp_parse(char **line, int *argc, char **argv);

/*
typedef int bool;
#define false 0
#define true  1
*/
#ifdef __cplusplus
#define __externC extern "C"
#else
#define __externC extern
#endif
// Also define externC for now - but it is deprecated
#define externC __externC

#ifndef CLI_COMMAND_SIZE
#define CLI_COMMAND_SIZE 20
#endif

typedef int cmd_fun(int argc, char *argv[]);
struct cmd {
    char    *str;
    char    *help;
    cmd_fun *fun;

};


#define INVALID_KEY	0
#define VALID_KEY	1

#define VKEY(x)		(0x100|(x))
#define VKEY_ESC	0x1B
#define VKEY_BS		0x08 //backspace
#define VKEY_TAB	0x09
#define VKEY_UP		VKEY(1)
#define VKEY_DOWN	VKEY(2)
#define VKEY_LEFT	VKEY(3)
#define VKEY_RIGHT	VKEY(4)
#define VKEY_PGUP	VKEY(5)
#define VKEY_PGDN	VKEY(6)
#define VKEY_HOME	VKEY(7)
#define VKEY_END	VKEY(8)
#define VKEY_F1		VKEY(0x10)
#define VKEY_F2		VKEY(0x11)
#define VKEY_F3		VKEY(0x12)
#define VKEY_F4		VKEY(0x13)
#define VKEY_F5		VKEY(0x14)
#define VKEY_F6		VKEY(0x15)
#define VKEY_F7		VKEY(0x16)
#define VKEY_F8		VKEY(0x17)
#define VKEY_F9		VKEY(0x18)
#define VKEY_F10	VKEY(0x19)
#define VKEY_F11	VKEY(0x1A)
#define VKEY_F12	VKEY(0x1B)

char * AASP_GetLine(char *string, int buf_len);
void aasp_start(void);
void aasp_shell(void);

#endif

