#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, TK_EQ, //空格,等于
  TK_DEC,TK_HEX,TK_REG,   //十进制,十六,寄存器
  TK_NEQ,TK_AND,TK_OR,    //不等于,与,或
  TK_NEG,TK_DEREF,        //负号,指针
  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},         // equal  
  {"!=",TK_NEQ},        //not equal
  {"0x0|0x[1-9A-Fa-f][0-9A-Fa-f]*",TK_HEX},//hex
  {"0|[1-9][0-9]*", TK_DEC},        // dec 
  {"\\$(eax|ecx|edx|ebx|esp|ebp|esi|edi|eip|ax|cx|dx|bx|sp|bp|si|di|al|cl|dl|bl|ah|ch|dh|bh)",TK_REG},//reg
  {"&&",TK_AND},      //and
  {"\\|\\|",TK_OR},   //or
  {"!",'!'},          //not

  {"-",'-'},            //minus
  {"\\*",'*'},          //mul
  {"\\/",'/'},          //dup
  {"\\(",'('},          //LB
  {"\\)",')'},          //RB
  
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        //由于数组的特性,此处如果以最大值32判断
        //则末尾补0将占据下一个数据的type,类型为enum
        //若enum最大值不超过0x00ffffff,保证后续第一个字节为\0
        //则可以如此判断,但逻辑不合格
        //此处应检测最大长度31,留1B补\0
        if(substr_len>=32){printf("Max of string is 31\n");return false;}//由于数组只有32B,为了补\0不能超过31B
        if(rules[i].token_type==TK_NOTYPE)break;
        tokens[nr_token].type=rules[i].token_type;
        switch (rules[i].token_type) {
          case TK_DEC:
            strncpy(tokens[nr_token].str,substr_start,substr_len);
            *(tokens[nr_token].str+substr_len)='\0';
            //printf("\n%s",tokens[nr_token].str);
            //printf("\n%d",substr_len);
            //printf("\n%d",*(tokens[nr_token].str+substr_len+3));
            break;
          case TK_HEX:
            strncpy(tokens[nr_token].str,substr_start+2,substr_len-2);
            *(tokens[nr_token].str+substr_len-2)='\0';
            break;
          case TK_REG:
            strncpy(tokens[nr_token].str,substr_start+1,substr_len-1);
            *(tokens[nr_token].str+substr_len-1)='\0';
            break;
          //default: TODO();
        }
        nr_token+=1;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
  
  //printf("\n%s",tokens[0].str);
  //for(int i=0;i<nr_token;i++)
  //  printf("%s\n",tokens[i].str);
  return true;
}

int eval(int p,int q);//计算表达式

int check_parentheses(int p,int q);//检测括号匹配

bool check_expr();//检测全部表达式合法

int dominant_op(int p,int q);

void check_neg();

bool expr_error=0;

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  
  /* TODO: Insert codes to evaluate the expression. */
  if(!check_expr()){
    *success = false;
    return 0;
  }//判断表达式是否合法

  check_neg();//检查负号

  uint32_t res=eval(0,nr_token-1);

  if(expr_error){
    *success = false;
    return 0;
  }

  *success=true;
  
  return res;
}

void check_neg(){
  
}

bool check_expr(){
  //检查全表达式是否括号匹配
  int count=0;
  for(int i=0;i<nr_token;i++){
    if(tokens[i].type=='(')count++;
    else if(tokens[i].type==')'){
      if(count!=0)count--;
      else return 0;
    }

    if(tokens[i].type=='-')
      if(tokens[i-1].type!=TK_DEC) tokens[i].type=TK_NEG;
    if(tokens[i].type=='*')
      if(tokens[i-1].type!=TK_DEC) tokens[i].type=TK_DEREF;
  }
  //计数法判断
  if(count==0)return 1;
  return 0;
}

int eval(int p,int q){
  if(p>q) {
    //bad
    //printf("error situation in eval p>q\n");
    expr_error=1;
    return 0;
  }
  else if(p==q){
    //single
    //printf("single number %s\n",tokens[p].str);
    int num;
    switch(tokens[p].type){
      case TK_DEC:
        sscanf(tokens[p].str,"%d",&num);
        return num;
      case TK_HEX:
        sscanf(tokens[p].str,"%x",&num);
        return num;
      case TK_REG:
        if(strcmp(tokens[p].str,"eip")==0)
          return cpu.eip;
        for(int i=0;i<8;i++){
          if(strcmp(tokens[p].str,regsl[i])==0)
            return reg_l(i);
          if(strcmp(tokens[p].str,regsw[i])==0)
            return reg_w(i);
          if(strcmp(tokens[p].str,regsb[i])==0)
            return reg_b(i);
        }
        printf("wrong reg in eval\n");
        assert(0);
    }
    return atoi(tokens[p].str);
  }
  else if(check_parentheses(p,q)==1){
    //printf("find parentheses\n");
    return(eval(p+1,q-1));
  }
  else{
    int value_in_reg;
    int res;
    vaddr_t addr;
    int op=dominant_op(p,q);
    //printf("this op is %c\n",tokens[op].type);

    switch (tokens[op].type)
    {
      case TK_NEG:
        return -eval(p+1,q);
      case TK_DEREF:
        addr=eval(p+1,q);
        value_in_reg=vaddr_read(addr,4);
        printf("addr=%u(0x%x)-->value=%d(0x%08x)\n",addr,addr,value_in_reg,value_in_reg);
        return value_in_reg;
      case '!':
        res=eval(p+1,q);
        return res?0:1;
    }


    int val1=eval(p,op-1);
    int val2=eval(op+1,q);
    switch (tokens[op].type)
    {
      case '+':return val1+val2;
      case '-':return val1-val2;
      case '*':return val1*val2;
      case '/':if(val2!=0)return val1/val2;else printf("Divided by 0");assert(0);
      case TK_EQ:return val1==val2;
      case TK_NEQ:return val1!=val2;
      case TK_AND:return val1&&val2;
      case TK_OR:return val1||val2;
      default:printf("error situation in evald p<q\n");
    }
    
  }
  return 0;
}

int check_parentheses(int p,int q){
  if(p>=q){printf("error situation in checkp p>=q\n");return -1;}//检测pq,不合法返回-1
  if(tokens[p].type!='('||tokens[q].type!=')')return 0;//检测pq是否为括号

  int count=0;
  for(int i=p+1;i<q;i++){
    if(tokens[i].type=='(')count++;
    else if(tokens[i].type==')'){
      if(count!=0)count--;
      else return 0;
    }
  }
  //计数法判断
  if(count==0)return 1;
  return 0;
}

int dominant_op(int p,int q){
  if(p>=q){printf("error situation in dominant p>=q\n");return -1;}//检测pq,不合法返回-1

  int pos[5]={-1,-1,-1,-1,-1};
  int count=0;

  for(int i=p;i<=q;i++){
    if(tokens[i].type=='(')count++;
    else if(tokens[i].type==')')count--;

    if(count==0){
      switch (tokens[i].type)
      {
        case TK_NEG:
        case TK_DEREF:
        case '!':
          pos[4]=i;
          break;
        case '/':
        case '*':
          pos[3]=i;
          break;
        case '+':
        case '-':
          pos[2]=i;
          break;
        case TK_EQ:
        case TK_NEQ:
          pos[1]=i;
          break;
        case TK_AND:
        case TK_OR:
          pos[0]=i;
          break;
      }
    }
  }
  //printf("%d%d",pos1,pos2);

  for(int i=0;i<5;i++)
    if(pos[i]!=-1)
      return pos[i];

  printf("wrong dominant op\n");
  assert(0);
}