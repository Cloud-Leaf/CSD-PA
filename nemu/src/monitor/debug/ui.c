#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");//通过readline来读取，提示符为（nemu）

  if (line_read && *line_read) {
    add_history(line_read);//readline中存在history管理
  }

  return line_read;//返回输入字符串
}

static int cmd_c(char *args) {
  cpu_exec(-1);//执行0xffffffff条指令,执行全部
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_help(char *args);

static int cmd_p(char *args);

static int cmd_x(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Let the program step through N instructions and pause execution. When N is not given, the default value is 1", cmd_si },
  { "info", "Input info r to print register status;info w to print Monitoring point information", cmd_info },
  { "p", "Expression evaluation", cmd_p },
  { "x", "Scan memory", cmd_x },
  { "w", "Set monitoring points", cmd_w },
  { "d", "Delete monitoring points", cmd_d },
  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_p(char *args){
  bool s;
  uint32_t res=expr(args,&s);
  if(s)printf("value:%d\n",res);
  else printf("error expr\n");

  return 0;
}

static int cmd_x(char *args){
  if(args==NULL){printf("please input args\n");return 0;}
  int N=0;//打印数量
  char *arg = strtok(NULL, " ");
  N=atoi(arg);
  if(N<=0) {printf("please input a number greater than 0\n");return 0;}

  vaddr_t addr;//起始地址
  bool s;
  arg = strtok(NULL, " ");
  addr=expr(arg,&s);

  if(!s){printf("bad expression!\n");return 0;}

  //仅0x版本
  //int ret=sscanf(args,"%d 0x%x",&N,&addr);//获取参数,默认0x...输入
  //if(ret<=0){
  //  printf("args error\n");
  //  return 0;
  //};
  //printf("%d%x",N,addr);

  //for(int i=0;i<N;i++){
  //  if(i%4==0)printf("\n%#010x:\t0x%02x",addr+i,vaddr_read(addr+i,1));
  //  else printf("\t0x%02x",vaddr_read(addr+i,1));
  //}//利用vaddr_read,从arg1读arg2个字节然后返回

  for(int i=0;i<N;i++){
    printf("\n%#010x:\t0x%08x",addr+4*i,vaddr_read(addr+4*i,4));
  }//利用vaddr_read,从arg1读arg2个字节然后返回


  printf("\n");

  return 0;
}

static int cmd_w(char *args){
  new_wp(args);
  return 0;
}

static int cmd_d(char *args){
  int num=0;
  int no=sscanf(args,"%d",&num);

  if(no<=0){
    printf("args error\n");
    return 0;
  }

  int r=free_wp(num);
  if(r==false)
    printf("error, no watchpoint %d\n",num);
  else
    printf("success delete watchpoint %d\n",num);

  return 0;
}

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

static int cmd_si(char *args) {
  char *arg = strtok(NULL, " ");
  uint64_t N=1;//默认执行1条

  if(arg!=NULL){
    if(atoi(arg)<=0){
      printf("args error\n");
      return 0;
    }//禁止小于=0的数
    N=atoi(arg);
  }//更改N的值,若有输入
  
  cpu_exec(N);//玩儿去
  return 0;
}

static int cmd_info(char *args){
  char *arg = strtok(NULL, " ");
  if(strcmp(arg,"r")==0){
    printf("eax:%#010x\tebx:%#010x\tecx:%#010x\tedx:%#010x\n",cpu.eax,cpu.ebx,cpu.ecx,cpu.edx);
    printf("esp:%#010x\tebp:%#010x\tesi:%#010x\tedi:%#010x\n",cpu.esp,cpu.ebp,cpu.esi,cpu.edi);
    printf("eip:%#010x\n",cpu.eip);printf("eflags.zf:%#010x\n",cpu.eflags.ZF);
    printf("CR0:%#010x\tCR3:%#010x\n",cpu.CR0,cpu.CR3);
  }
  else if(strcmp(arg,"w")==0){
    print_wp();
    return 0;
  }
  else printf("Unknown arg '%s'\n", arg);
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }//进入匹配模式

  while (1) {
    char *str = rl_gets();//获取输入字符
    char *str_end = str + strlen(str);//字符串末尾

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");//分词，指向分词后的第一个词，再次执行则为第二个。。。
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;//指令后接参数
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
      }//根据指令在table中依次对比决定执行哪个指令
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
