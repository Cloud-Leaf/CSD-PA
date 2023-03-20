#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;
static int next_p=0;
static WP *temp;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
    wp_pool[i].old_v=0;
    wp_pool[i].hit=0;
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(char *args){
  if(free_==NULL){printf("no free watchpoint\n");assert(0);}//没有空间

  //检查并计算表达式,若不合法则不分陪
  int value;
  bool s;
  value=expr(args,&s);
  if(s==false){printf("please check your expression\n");return NULL;}

  //分配新wp
  WP* wp=free_;
  free_=free_->next;
  //初始化
  wp->NO=next_p++;
  wp->next=NULL;
  strcpy(wp->expr,args);
  wp->hit=0;
  wp->old_v=value;

  //加入监视点链表
  temp=head;
  if(head==NULL)head=wp;
  else {
    while(temp->next!=NULL)
      temp=temp->next;
    temp->next=wp;
  }
  
  printf("Success set watchpoint %d, old value: %d\n",wp->NO,wp->old_v);
  return wp;
}

bool free_wp(int no){
  WP *wp=NULL;
  if(head==NULL){printf("no watchpoint\n");return false;}//head不能为空

  if(head->NO==no){wp=head;head=head->next;}//检查head是不是符合
  else{
    temp=head;
    while(temp->next!=NULL&&temp!=NULL){
      if(temp->next->NO==no){
        wp=temp->next;
        temp->next=temp->next->next;
        break;
      }
      temp=temp->next;
    }
  }//其他

  //更新链表
  if(wp!=NULL){
    wp->next=free_;
    free_=wp;
    return true;
  }
  return false;
}

void print_wp(){
  if(head==NULL){printf("no watchpoint\n");return;}

  printf("watchpoints:\n");
  printf("NO.       expr        hit\n");
  temp=head;
  while(temp!=NULL){
    printf("%d        %s        %d\n",temp->NO,temp->expr,temp->hit);
    temp=temp->next;
  }
}

bool watch_wp(){
  bool s;
  int res;
  if(head==NULL)return false;//没变化

  bool ischange=0;

  temp=head;
  while(temp!=NULL){
    res=expr(temp->expr,&s);
    if(res!=temp->old_v){
      temp->hit++;
      printf("Hardware watchpoint %d:%s\n",temp->NO,temp->expr);
      printf("Old value:%d\nNew value:%d\n\n",temp->old_v,res);
      temp->old_v=res;
      ischange=1;//发生变化
    }
    temp=temp->next;
  }
  return ischange;
}


