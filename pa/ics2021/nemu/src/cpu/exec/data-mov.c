#include "cpu/exec.h"

make_EHelper(mov) {
  operand_write(id_dest, &id_src->val);
  print_asm_template2(mov);
}

make_EHelper(push) {
  rtl_push(&id_dest->val);

  print_asm_template1(push);
}

make_EHelper(pop) {
  rtl_pop(&t0); // 将M[R[esp]]的值赋给t0
  operand_write(id_dest, &t0); // 将t0 的值送到指定寄存器中
  print_asm_template1(pop);
}

make_EHelper(pusha) {
  if (decoding.is_operand_size_16) {
    t0 = reg_w(R_SP);
    rtl_push((rtlreg_t *)&reg_w(R_AX));
    rtl_push((rtlreg_t *)&reg_w(R_CX));
    rtl_push((rtlreg_t *)&reg_w(R_DX));
    rtl_push((rtlreg_t *)&reg_w(R_BX));
    rtl_push(&t0);
    rtl_push((rtlreg_t *)&reg_w(R_BP));
    rtl_push((rtlreg_t *)&reg_w(R_SI));
    rtl_push((rtlreg_t *)&reg_w(R_DI));
  }
  else {
    t0 = reg_w(R_ESP);
  	rtl_push(&cpu.eax);
  	rtl_push(&cpu.ecx);
  	rtl_push(&cpu.edx);
  	rtl_push(&cpu.ebx);
  	rtl_push(&t0);
 	  rtl_push(&cpu.ebp);
  	rtl_push(&cpu.esi);
  	rtl_push(&cpu.edi);
  }

  print_asm("pusha");
}

make_EHelper(popa) {
  if (decoding.is_operand_size_16) {
    rtl_pop((rtlreg_t*)&reg_w(R_DI));
    rtl_pop((rtlreg_t*)&reg_w(R_SI));
    rtl_pop((rtlreg_t*)&reg_w(R_BP));
    rtl_pop(&t0);
    rtl_pop((rtlreg_t*)&reg_w(R_BX));
    rtl_pop((rtlreg_t*)&reg_w(R_DX));
    rtl_pop((rtlreg_t*)&reg_w(R_CX));
    rtl_pop((rtlreg_t*)&reg_w(R_AX));
  }
  else {
    rtl_pop(&cpu.edi);
    rtl_pop(&cpu.esi);
    rtl_pop(&cpu.ebp);
    rtl_pop(&t0); 
    rtl_pop(&cpu.ebx);
    rtl_pop(&cpu.edx);
    rtl_pop(&cpu.ecx);
    rtl_pop(&cpu.eax);
  }

  print_asm("popa");
}

make_EHelper(leave) {
  if (decoding.is_operand_size_16) {
    rtl_mv((rtlreg_t *)&reg_l(R_SP), (rtlreg_t *)&reg_l(R_BP));
    rtl_pop((rtlreg_t *)&reg_w(R_BP));
  }
  else {
    rtl_mv((rtlreg_t *)&reg_l(R_ESP), (rtlreg_t *)&reg_l(R_EBP));
    rtl_pop((rtlreg_t *)&reg_l(R_EBP));
  }

  print_asm("leave");
}

make_EHelper(cltd) { // CWD & CDQ
  
  if (decoding.is_operand_size_16){
    short t = reg_w(R_AX);
    if (t < 0){
      reg_w(R_DX) = 0xffff;
    }
    else reg_w(R_DX) = 0;
  }
  else {
    int t = reg_l(R_EAX);
    if(t < 0){
      reg_l(R_EDX) = 0xffffffff;
    }
    else reg_l(R_EDX) = 0;
  }

  print_asm(decoding.is_operand_size_16 ? "cwtl" : "cltd");
}

make_EHelper(cwtl) { // CBW & CWDE
  if (decoding.is_operand_size_16) {
    rtl_sext((rtlreg_t *)&reg_w(R_AX), (rtlreg_t *)&reg_b(R_AL), 1);
  }
  else {
    rtl_sext((rtlreg_t *)&reg_l(R_EAX), (rtlreg_t *)&reg_w(R_AX), 2);
  }

  print_asm(decoding.is_operand_size_16 ? "cbtw" : "cwtl");
}

make_EHelper(movsx) {
  id_dest->width = decoding.is_operand_size_16 ? 2 : 4;
  rtl_sext(&t2, &id_src->val, id_src->width);
  operand_write(id_dest, &t2);
  print_asm_template2(movsx);
}

make_EHelper(movzx) {
  id_dest->width = decoding.is_operand_size_16 ? 2 : 4;
  operand_write(id_dest, &id_src->val);
  print_asm_template2(movzx);
}

make_EHelper(movsb){
  rtl_lm(&t0,&cpu.esi,1);
  rtl_sm(&cpu.edi,1,&t0);
  cpu.esi+=1;
  cpu.edi+=1;
  print_asm("movsb");

}

make_EHelper(lea) {
  rtl_li(&t2, id_src->addr);
  operand_write(id_dest, &t2);
  print_asm_template2(lea);
}
