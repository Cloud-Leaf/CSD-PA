#include "common.h"
#include "syscall.h"

int sys_none();
void sys_exit(int a);

_RegSet* do_syscall(_RegSet *r) {
  uintptr_t a[4];
  a[0] = SYSCALL_ARG1(r);
  a[1] = SYSCALL_ARG2(r);
  a[2] = SYSCALL_ARG3(r);
  a[3] = SYSCALL_ARG4(r);

  switch (a[0]) {
    case SYS_none:SYSCALL_ARG1(r)=sys_none();break;
    case SYS_exit:sys_exit(a[1]);break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

  return NULL;
}

int sys_none(){
  return 1;
}

void sys_exit(int a){
  _halt(a);
}