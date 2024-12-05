#include "blade_conf.h"
#ifdef BLADE_CLI
#include "aasp_shell.h"

int AASP_DebugLevel = DBGLVL_HIGH ; 
int uart_read_cmdkey(int * key);
extern char UART_Delay_ReceiveChar(void);
extern char UART_ReceiveChar(void);
extern int Cli_Init(void);



#define KEYHISTORYMAX	5
typedef struct cmd_history CMD_HISTORY;
typedef struct cmd_history *PCMD_HISTORY;
struct cmd_history
{
	char cmd[128];
	CMD_HISTORY *next;
	CMD_HISTORY *pre;
};

#ifndef BLADE_CLI_CUT	
CMD_HISTORY cmd_history_array[KEYHISTORYMAX];
PCMD_HISTORY cmd_cur=cmd_history_array;
PCMD_HISTORY cmd_show=cmd_history_array;
#endif

void send_backspace(int count);
void copy_cmd(char * line);
int show_pre_cmd(char * string);
int show_next_cmd(char * string);
extern Subcmd *CliCommandList;


//
// Scan through an input line and break it into "arguments".  These
// are space delimited strings.  Return a structure which points to
// the strings, similar to a Unix program. Multiple commands in the line
// are separated by ; similar to sh. If we find a semi we stop processing the 
// line, terminate the current command with a null and return the start 
// of the next command in *line. parse() can then be called again to 
// process the next command on the line.
// Note: original input is destroyed by replacing the delimiters with 
// null ('\0') characters for ease of use.
//
//Subcmd *
void
aasp_parse(char **line, int *argc, char **argv)
{
    char *cp = *line;
    char *pp;
    int indx = 0;
    int semi = 0;

    argv[0]=" ";

    while (*cp) {
        // Skip leading spaces
        while (*cp && *cp == ' ') cp++;
        if (!*cp) {
            break;  // Line ended with a string of spaces
        }
	if (*cp == ';') {
            *cp = '\0';
            semi=1;
            break;
	}
        if (indx < MAX_ARGV) {
            argv[indx++] = cp;
        } else {
            iprintf("Too many arguments - stopped at: '%s'\n", cp);
        }
        while (*cp) {
            if (*cp == ' ') {
                *cp++ = '\0';
                break;
            } else if (*cp == ';') {
                break;
	    } else if (*cp == '"') {
                // Swallow quote, scan till following one
                if (argv[indx-1] == cp) {
                    argv[indx-1] = ++cp;
                }
                pp = cp;
                while (*cp && *cp != '"') {
                    if (*cp == '\\') {
                        // Skip over escape - allows for escaped '"'
                        cp++;
                    }
                    // Move string to swallow escapes
                    *pp++ = *cp++;
                }
                if (!*cp) {
                    iprintf("Unbalanced string!\n");
                } else {
                    if (pp != cp) *pp = '\0';
                    *cp++ = '\0';
                    break;
                }
            } else {
                cp++;
            }
        }
    }
    if (semi) {
        *line = cp + 1;
    } else {
        *line = cp;
    }
    *argc = indx;
    
    //return cmd_search(__AASP_CMD_TAB__, __AASP_CMD_TAB_END__, argv[0]);
    //return cmd_search(__AASP_CMD_TAB__, &__AASP_CMD_TAB_END__, argv[0]);
}


Subcmd * subcmd_search(Subcmd *tab,char *arg)
{
    int cmd_len;
    Subcmd *cmd, *cmd2;
    // Search command table
    cmd_len = strlen(arg);
    cmd = tab;
    

    if(tab->str==0){
        iprintf("(tab==tabend) command table is empty!\n");
    	return (Subcmd *)0;
       
    }
    while (cmd->str != 0) {
        //if (strncasecmp(arg, cmd->str, cmd_len) == 0) {
        if (strncmp(arg, cmd->str, cmd_len) == 0) {
            if (strlen(cmd->str) > cmd_len) {
                // Check for ambiguous commands here
                // Note: If there are commands which are not length-unique
                // then this check will be invalid.  E.g. "du" and "dump"
                int first = true;
                cmd2 = tab;
                //while (cmd2 != tabend) {
                while (cmd2->str != 0) {
                    if ((cmd != cmd2) && 
                        //(strncasecmp(arg, cmd2->str, cmd_len) == 0)) {
                        (strncmp(arg, cmd2->str, cmd_len) == 0)) {
                        if (first) {
                            iprintf("Ambiguous command '%s', choices are: %s", 
                                        arg, cmd->str);
                            first = false;
                        }
                        iprintf(" %s", cmd2->str);
                    }
                    cmd2++;
                }
                if (!first) {
                    // At least one ambiguity found - fail the lookup
                    iprintf("\n");
                    return (Subcmd *)0;
                }
            }
            return cmd;
        }
        cmd++;
    }
    return (Subcmd *)0;
}

#ifndef BALDE_CLI_CUT	
//#define INVALID_KEY	0
//#define VALID_KEY	1
int uart_read_cmdkey(int * key)
{
	char c;
#if 1
	c=UART_Delay_ReceiveChar();
	switch(c)
	{
		case 'O':
 		       c=UART_Delay_ReceiveChar();
			switch(c)
			{
				case 'P':
					*key=VKEY_F1;
					return VALID_KEY;
				case 'Q':
					*key= VKEY_F2;
					return VALID_KEY;
				case 'R':
					*key= VKEY_F3;
					return VALID_KEY;
				case 'S':
					*key= VKEY_F4;
					return VALID_KEY;		
			}
		    break;
		case 0x5B:
 			c=UART_Delay_ReceiveChar();
			switch(c)
			{	
				//Uart_SendByte(c);
				case 0x41:
					*key= VKEY_UP;
					return VALID_KEY;
				case 0x42:
					*key= VKEY_DOWN;
					return VALID_KEY;
				case 0x43:
					*key= VKEY_RIGHT;
					return VALID_KEY;
				case 0x44:
					*key= VKEY_LEFT;
					return VALID_KEY;		
				default:
					*key=c;
					return INVALID_KEY;
			}
		   // break; warning:  #111-D: statement is unreachable
		default:
			//*key=c;
			return INVALID_KEY;
	}
#endif	
	return 0;
}


void save_cmd(char * line)
{
	//PCMD_HISTORY pcmd=cmd_history_array;
	//int i=0;

	/*
	while(i<sizeof(cmd_history_array))
	{
		if (0==strcmp(line,pcmd->cmd))
		{
			return;
		}
		i++;
		pcmd=pcmd->next;
	}
	*/
	memset(cmd_cur->cmd,0,sizeof(cmd_cur->cmd));
	strncpy(cmd_cur->cmd,line,sizeof(cmd_cur->cmd)-1);
	cmd_show=cmd_cur;
	cmd_cur=(PCMD_HISTORY)cmd_cur->next;
	return;
}
#endif

void send_backspace(int count)
{
	while(count>0)
	{
		iprintf("\b \b");
		count--;
	}
}
#ifndef BALDE_CLI_CUT	
int show_pre_cmd(char * string)
{
	int len;
	char * pstr=cmd_show->cmd;
	int i=0;

	// find cmd
	while( *pstr == '\0' && i<sizeof(cmd_history_array))
	{
		cmd_show = cmd_show->pre;
		pstr=cmd_show->cmd;
		i++;
	}
	len=strlen(pstr);
	while(*pstr != '\0')
	{
		*string++=*pstr;
		uputc(*pstr);
		pstr++;
	}
	cmd_show = cmd_show->pre;
	return len;
}

int show_next_cmd(char * string)
{
	int len;
	char * pstr = cmd_show->cmd;
	int i=0;
	
	// find cmd
	while( *pstr == '\0' && i<sizeof(cmd_history_array))
	{
		cmd_show = cmd_show->pre;
		pstr=cmd_show->cmd;
		i++;
	}
	len=strlen(pstr);
	while(*pstr != '\0')
	{
		*string++=*pstr;
		uputc(*pstr);
		pstr++;
	}
	cmd_show = cmd_show->next;
	return len;
}
#endif

void cmd_init(void)
{
#ifndef BALDE_CLI_CUT	
	int i =0;

	for(i =0; i<KEYHISTORYMAX; i++)
	{
		memset(&cmd_history_array[i],0,sizeof(CMD_HISTORY));
	}
	
	//bidirection linklist init	
	for(i =0; i<KEYHISTORYMAX; i++)
	{
		if(i == KEYHISTORYMAX-1)
			cmd_history_array[i].next =  cmd_history_array;
		else
			cmd_history_array[i].next = &cmd_history_array[i+1];
	}
	for(i=0; i<KEYHISTORYMAX; i++)
	{
		if(i == 0)
			cmd_history_array[i].pre = &cmd_history_array[KEYHISTORYMAX-1];
		else
			cmd_history_array[i].pre = &cmd_history_array[i-1];
	}
#endif	
}

/////rewrite for no os system
void aasp_init(void)
{
	cmd_init();
	Cli_Init();
	return ;
}

char * AASP_GetLine_nos(char *string, int buf_len, char* cmd)
{
	#ifndef BOOTLOADER	
	char *string2=string;
	int rt;
	int key;
	#endif
	int len=0;
	char c;
	

	c = UART_ReceiveChar();
	if(c == 0xff)
	{
		return NULL;
	}
	
	if(c == 0)
	{
		return NULL;
	}
		
	if(c!='\r')	
	{
	
		if(len<buf_len)
		{
			switch(c)
			{
			case VKEY_BS:
				if(	strlen(cmd)>0 )
				{
					send_backspace(1);
					string--;
					*string = 0;
					len--;
					return string;
				}
				break;
			case VKEY_TAB:
				;
				break;

			case VKEY_ESC:
			#ifndef BALDE_CLI_CUT	
				rt = uart_read_cmdkey(&key);
				if(rt==VALID_KEY)  //up,down,right,left,f1...
				{
					//Uart_SendByte(key);
					switch(key)
					{
					case VKEY_UP:
					case VKEY_RIGHT:
						send_backspace(string-cmd);
						memset(cmd,0,128);
						len = show_pre_cmd(cmd);
						string=cmd+len;
						return string;
					case VKEY_DOWN:
					case VKEY_LEFT:
						send_backspace(string-cmd);
						memset(cmd,0,128);
						len = show_next_cmd(string2);
						string=cmd+len;
						return string;
					case VKEY_F1:
						//do nothing;
						break;
					default:
						//do nothing;
						;
					}
				}
				else  //just char
				{
					//press ESC will erase current cmd
					send_backspace(len);
					memset(string2,0,buf_len);
					*string++=key;
					len = 1;
					uputc(key);

				}
			#endif
				//copy_cmd(string2);
				break;
				
			default:
				*string++=c;
				len++;
				uputc(c);
				return string;
				//break;
			}
		}

	}
	
	
	if(c=='\r')	
	{
		*string='\r';
		uputc('\r');
		uputc('\n');
		return string;
	}
	return NULL;
}

#ifdef BLADE_CAN_CLI
extern uint32_t can_cli_index;
extern uint8_t can_cli_buf[1024];
extern uint32_t can_cli_tick;
#endif
void aasp_shell_nos(void)
{
	int argc;
	char *argv[MAX_ARGV]={0};
	static int cmd_step = 0;
	static char line[128];
#ifndef BALDE_CLI_CUT	
	char line_temp[128]={0};
#endif
	char *command;
	Subcmd *cmd;
	static char* pchar;
	char *ptr;
	//first or last is \r
#ifdef BLADE_CAN_CLI
	{								//50ms周期性，清空发送缓存区
		static uint32_t r_index=0;			//读指�?
		if(can_cli_index>0)
		{
			int subValue=can_cli_index-r_index;
			if(subValue>8){
				DrvCan1SendDataEx((CAN_CLI_TRANS_FUNCID+CAN_CLI_NODEID),can_cli_buf+r_index,8);
				r_index=r_index+8;
			}else{
				DrvCan1SendDataEx((CAN_CLI_TRANS_FUNCID+CAN_CLI_NODEID),can_cli_buf+r_index,subValue);
				r_index=0;
				can_cli_index=0;
				memset(can_cli_buf,0,sizeof(can_cli_buf));
			}
		}
	}
#endif
	if(cmd_step == 0)
	{
		#ifdef BOOTLOADER
					
		iprintf("Geek_boot>");
		#else
					
		iprintf("Borui_Zhixin>");
		
		
		#endif
		pchar = line;
		memset(line, 0, sizeof(line));
		cmd_step = 1;
	}
	
	
	while(1)
	{
		// Get command line from the fifo
		ptr = (char *)AASP_GetLine_nos(pchar, sizeof(line)-(pchar-line), line);   
			
		if (ptr == NULL)
			return;
		else 
			pchar = ptr;
		
		if(*ptr!='\r')
			continue;
		else
			*ptr='\0';
			cmd_step = 0;
			break;
	}

	command = (char *)&line;
#ifndef BALDE_CLI_CUT	
	strncpy(line_temp, line, sizeof(line_temp)-1);
#endif
	aasp_parse(&command, &argc, &argv[0]); 


	if(argc==0)
		return;//continue;

	if ((cmd = (Subcmd *)subcmd_search((Subcmd *)CliCommandList, argv[0])) != (Subcmd *)0) 
	{
		(cmd->fun)(argc, argv);
	#ifndef BALDE_CLI_CUT	
		save_cmd(line_temp);
	#endif	
	} 
	else
	{
		iprintf("** Error: Illegal command: \"%s\"\r\n", argv[0]);
	}
}
#endif
