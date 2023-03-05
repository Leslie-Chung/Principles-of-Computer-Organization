#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);
static int cmd_si(char *args);
void reg_display();
static int cmd_info(char *args);
void byteSequence_dispaly(uint32_t data);
static int cmd_x(char *args);
static int cmd_p(char * args);

static int cmd_w(char *args);
static int cmd_b(char *args);
static int cmd_d(char *args);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_b(char *args){
	char *arg1 = strtok(NULL, " ");
	if(arg1 == NULL){
		printf("A parameter is missing!\n");
		return 0;
	}
	char *arg2 = strtok(NULL, " ");
	while(arg2 != NULL){
		strcat(arg1, arg2);
		arg2 = strtok(NULL, " ");
	}

	int NO = set_breakpoint(arg1);
	if(NO != -1){
		bool success = true;
		printf("Set breakpoint #%d\n", NO);
		uint32_t address = expr(args, &success);
		printf("address   = %#010x\n", address);
		printf("old value = 0x%08x\n", vaddr_read(address, 4));
	}
	return 0;
}

static int cmd_d(char *args)
{
	char *arg = strtok(NULL, " ");
	if(arg == NULL){
		printf("A parameter is missing!\n");
		return 0;        
	}
	int NO = atoi(args);
	if(NO < 0){
		printf("Invalid arguments for d!\n");
		return 0;           
	}	
	if(delete_watchpoint(NO)){
		printf("Watchpoint/breakpoint %d deleted\n", NO);
	}
	else{
		printf("Watchpoint/breakpoint %d not found\n", NO);
	}
	return 0;
}

static int cmd_w(char *args){
	char *arg1 = strtok(NULL, " ");
	if(arg1 == NULL){
		printf("A parameter is missing!\n");
		return 0;
	}
	char *arg2 = strtok(NULL, " ");
	while(arg2 != NULL){
		strcat(arg1, arg2);		
		arg2 = strtok(NULL, " ");
	}

	int NO = set_watchpoint(arg1);
	if(NO != -1){
	  bool success = true;
		printf("Set watchpoint #%d\n", NO);
 	  printf("expr      = %s\n", args);
		printf("old value = 0x%08x\n", expr(args, &success));
	}
	return 0;
}

static int cmd_si(char *args){
	char *arg = strtok(NULL, " ");
	int n; //因为si指令已经被捕获，所以只需读出数字即可，并将函数添加到cmd_table
	if (arg){
		n = atoi(arg);
		if (n < -1) {
			printf("Input Digit Error!\n");
			return 0;
		}
	}
	else n = 1;//缺省
	cpu_exec(n);
	return 0;
}

void reg_display(){
 	int i;
	for (i = R_EAX; i <= R_EBX; i++){
		printf("%s:\t0x%-8x\t%u\n", regsl[i], reg_l(i), reg_l(i));	
		printf("%s:\t0x%-8x\t%u\n", regsw[i], reg_w(i), reg_w(i));
		printf("%s:\t0x%-8x\t%u\n", regsb[i], reg_b(i), reg_b(i));
		printf("%s:\t0x%-8x\t%u\n", regsb[i + 4], reg_b((i + 4)), reg_b((i + 4)));
		printf("\n");
	}
	printf("\n");
	for (; i <= R_EDI; ++i)	{
		printf("%s:\t0x%-8x\t%u\n", regsl[i], reg_l(i), reg_l(i));
		printf("%s:\t0x%-8x\t%u\n", regsw[i], reg_w(i), reg_w(i));
		printf("\n");
 	}
	printf("\n");
	printf("eip:\t0x%-8x\t%u\n", cpu.eip, cpu.eip);
	printf("ip:\t0x%-8x\t%u\n", cpu.ip, cpu.ip);
	printf("\n");
	printf("eflags:\t0x%-8x\t%u\n", cpu.eflags, cpu.eflags);
	printf("flags:\t0x%-8x\t%u\n", cpu.flags, cpu.flags);
}

static int cmd_info(char *args){
	char *arg = strtok(NULL, " ");
	if(arg == NULL){
		printf("A parameter is missing!\n");
		return 0;
	}
	if (strcmp(arg, "r") == 0){
		reg_display();	
	}
	else if (strcmp(arg, "w") == 0)	{
		list_watchpoint();
	}
	else{
		printf("Unknown command '%s'\n", arg); 
	}

	return 0;
}

void byteSequence_dispaly(uint32_t data){
	printf("   ...  ");
 	uint32_t byte[4];
	byte[0] = data & 0x000000ff;
	byte[1] = (data & 0x0000ff00) >> 8;
	byte[2] = (data & 0x00ff0000) >> 16;
	byte[3] = (data & 0xff000000) >> 24;
	int i;
	for(i = 0; i < 4; i++){
		printf("%02x ",byte[i]);
	}
	printf("\n");
}

static int cmd_x(char *args){
 	char *arg1 = strtok(NULL, " ");
	char *arg2 = strtok(NULL, " ");
	if (arg1 == NULL || arg2 == NULL) {
		printf("A parameter is missing!\n");
		return 0;
	}
	int n = atoi(arg1); //读取要读取的次数
	if (n < 1){
		printf("Invalid arguments for x!\n");
		return 0;
	}
	char *arg3 = strtok(NULL, " ");	
	while(arg3 != NULL){
		strcat(arg2, arg3);
		arg3 = strtok(NULL, " ");
	}

	int i;
	uint32_t data, addr;
	bool success = true;
	addr = expr(arg2, &success);
	if(!success) return 0;
	printf("Address         Dword block  ...  Byte sequence\n");
	//循环使用 vaddr_read 函数来读取内存
	for (i = 1; i <= n; i++, addr += 4){
		data = vaddr_read(addr, 4);
		printf("0x%08x\t", addr);
		printf("0x%08x", data);
		byteSequence_dispaly(data);
	}
	return 0;
}

static int cmd_p(char *args){
	bool success = true;
	uint32_t value = expr(args,&success);
	if(success){
		printf("%u\n", value);
	}    
	return 0;
}

static int cmd_help(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  {"help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
	{ "si","Usage: si [N]\n"\
			    "     Execute the program with N(default: 1) step", cmd_si },
	{ "info", "Show information about registers with argument 'r' and show information about watchpoint with argument 'w'", cmd_info},
  { "x", "Usage: x [N] [EXPR]\n" \
			"    Calculate the value of the expression EXPR, and output N consecutive 4 bytes starting from EXPR in hexadecimal form", cmd_x },
	{ "p", "Usage: p [EXPR]\n" "    Calculate the value of the expression EXPR", cmd_p},
 	/* TODO: Add more commands */
	{ "w", "Usage: w [EXPR]\n" "    set watchpoint for the [EXPR].", cmd_w},
	{ "d", "usage: d [N]\n" "    delete watchpoint whose id is N", cmd_d},
  {"b", "b EXPR:set a breakpoint for eip as the value of EXPR", cmd_b},
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
