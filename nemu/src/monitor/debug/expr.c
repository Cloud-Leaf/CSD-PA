#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_INT,
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
  {"0|[1-9][0-9]*", TK_INT},        // int 
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
          case TK_INT:
            strncpy(tokens[nr_token].str,substr_start,substr_len);
            *(tokens[nr_token].str+substr_len)='\0';
            //printf("\n%s",tokens[nr_token].str);
            //printf("\n%d",substr_len);
            //printf("\n%d",*(tokens[nr_token].str+substr_len+3));
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
  return true;
}

int eval(int p,int q);//计算表达式

int check_parentheses(int p,int q);//检测括号匹配

int dominant_op(int p,int q);

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  *success=true;
  
  return eval(0,nr_token-1);;
}

int eval(int p,int q){
  if(p>q) {
    //bad
    //printf("error situation in eval p>q\n");
    assert(0);
    return 0;
  }
  else if(p==q){
    //single
    printf("single number %s\n",tokens[p].str);
    return atoi(tokens[p].str);
  }
  else if(check_parentheses(p,q)==1){
    printf("find parentheses\n");
    return(eval(p+1,q-1));
  }
  else{
    int op=dominant_op(p,q);
    printf("this op is %c\n",tokens[op].type);
    int val1=eval(p,op-1);
    int val2=eval(op+1,q);
    switch (tokens[op].type)
    {
    case '+':return val1+val2;
    case '-':return val1-val2;
    case '*':return val1*val2;
    case '/':if(val2!=0)return val1/val2;else printf("Divided by 0");assert(0);
    default:printf("error situation in evald p>=q\n");
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

  int pos1=-1;
  int pos2=-1;

  int count=0;
  for(int i=p;i<q;i++){
    if(tokens[i].type=='(')count++;
    else if(tokens[i].type==')')count--;

    if(count==0&&tokens[i].type!=TK_INT){
      if(tokens[i].type=='+'||tokens[i].type=='-')pos1=i;
      else if(tokens[i].type=='*'||tokens[i].type=='/')pos2=i;
    }
  }
  //printf("%d%d",pos1,pos2);
  return pos1==-1?pos2:pos1;

}