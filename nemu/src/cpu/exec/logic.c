#include "cpu/exec.h"

make_EHelper(test) {
  //TODO();

  rtl_and(&t2,&id_dest->val,&id_src->val);
  rtl_update_ZFSF(&t2,id_dest->width);
  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);

  print_asm_template2(test);
}

make_EHelper(and) {
  //TODO();
  rtl_and(&t2,&id_dest->val,&id_src->val);
  operand_write(id_dest,&t2);//写回寄存器1/修改eflags
  //SF ZF
  rtl_update_ZFSF(&t2,id_dest->width);//CF oF -o
  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);

  print_asm_template2(and);
}

make_EHelper(xor) {
  //TODO();

  rtl_xor(&t2,&id_dest->val,&id_src->val);
  operand_write(id_dest,&t2);

  rtl_update_ZFSF(&t2 ,id_dest->width);
  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);

  print_asm_template2(xor);
}

make_EHelper(or) {
  //TODO();

  rtl_or(&t2,&id_dest->val,&id_src->val);
  operand_write(id_dest,&t2);
  rtl_update_ZFSF(&t2,id_dest->width);
  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);


  print_asm_template2(or);
}

make_EHelper(sar) {
  //TODO();
  // unnecessary to update CF and OF in NEMU

  rtl_sext(&t2,&id_dest->val,id_dest->width);
  rtl_sar(&t2,&t2,&id_src->val);
  operand_write(id_dest,&t2);//写回寄存器
  rtl_update_ZFSF(&t2,id_dest->width);
  print_asm_template2(sar);
}

make_EHelper(shl) {
  //TODO();
  // unnecessary to update CF and OF in NEMU

  rtl_shl(&t2,&id_dest->val,&id_src->val);
  operand_write(id_dest,&t2);//写回寄存器

  rtl_update_ZFSF(&t2,id_dest->width);

  print_asm_template2(shl);
}

make_EHelper(rol) {
  int CL=id_src->val;//CL times
  
  rtl_li(&t0,id_dest->val);//t0中为初始值
  while(CL){
    rtl_msb(&t2,&t0,id_dest->width);//t2中为最高有效位
    rtl_shli(&t0,&t0,0x1);//t0*2
    rtl_add(&t0,&t0,&t2);//t0=t0+t2
    CL-=1;
  }
  if(id_src->val==1){
    // t2暂存高位
    rtl_msb(&t2,&t0,id_dest->width);
    rtl_eqi(&t2,&t2,cpu.eflags.CF);
    t2=t2==0?1:0;
    rtl_set_OF(&t2);
  }
  operand_write(id_dest,&t0);
  print_asm_template1(rol);
}

make_EHelper(shr) {
  //TODO();
  // unnecessary to update CF and OF in NEMU

  rtl_shr(&t2,&id_dest->val,&id_src->val);
  operand_write(id_dest,&t2);//写回寄存器

  rtl_update_ZFSF(&t2,id_dest->width);

  print_asm_template2(shr);
}

make_EHelper(setcc) {
  uint8_t subcode = decoding.opcode & 0xf;
  rtl_setcc(&t2, subcode);
  operand_write(id_dest, &t2);

  print_asm("set%s %s", get_cc_name(subcode), id_dest->str);
}

make_EHelper(not) {
  //TODO();

  rtl_not(&id_dest->val);
  operand_write(id_dest,&id_dest->val);

  print_asm_template1(not);
}
