#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  int old_v;//存放旧值
  char expr[32];//表达式
  int hit;//命中次数


} WP;

WP* new_wp(char *args);
bool free_wp(int no);
void print_wp();
bool watch_wp();




#endif
