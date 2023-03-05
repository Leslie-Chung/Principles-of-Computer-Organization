#include "cpu/exec.h"
#include "monitor/watchpoint.h"
#include "monitor/monitor.h"
void diff_test_skip_qemu();
void diff_test_skip_nemu();

make_EHelper(int3) {
	   /*int3 指令的执行函数	*/
	print_asm("Breakpoint (eip = %0#10x)", cpu.eip);
	printf("\33[1;31mnemu: HIT BREAKPOINT\33[0m at eip = %0#10x\n\n", cpu.eip);
	//上面两个输出仿照nemu/src/cpu/exec/special.c 的 make_EHelper(nemu_trap) 函数
	recover_int3();
	nemu_state = NEMU_STOP;
}

make_EHelper(lidt) {
  cpu.idtr.limit = vaddr_read(id_dest->addr, 2);
  if (decoding.is_operand_size_16)
    cpu.idtr.base = vaddr_read(id_dest->addr + 2, 3);
  else
    cpu.idtr.base = vaddr_read(id_dest->addr + 2, 4);

  print_asm_template1(lidt);
}

make_EHelper(mov_r2cr) {
  if(id_dest->reg == 0) {
  	cpu.cr0.val = id_src->val;
  }
  else if(id_dest->reg == 3) {
  	cpu.cr3.val = id_src->val;
  }else{
  	assert(0);
  }

  print_asm("movl %%%s,%%cr%d", reg_name(id_src->reg, 4), id_dest->reg);
}

make_EHelper(mov_cr2r) {
  if(id_src->reg == 0) {
    operand_write(id_dest, &cpu.cr0.val);
  }
  else if(id_src->reg == 3) {
  	operand_write(id_dest, &cpu.cr3.val);
  }
  else{
  	assert(0);
  }

  print_asm("movl %%cr%d,%%%s", id_src->reg, reg_name(id_dest->reg, 4));

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

void raise_intr(uint8_t NO, vaddr_t ret_addr);
make_EHelper(int) {
  raise_intr(id_dest->val, decoding.seq_eip);

  print_asm("int %s", id_dest->str);

#ifdef DIFF_TEST
  diff_test_skip_nemu();
#endif
}

make_EHelper(iret) {
  rtl_pop(&decoding.jmp_eip);
  rtl_pop(&t0);
  cpu.cs = (uint16_t)t0;
  rtl_pop(&cpu.eflags.value);
  decoding.is_jmp = 1;

  print_asm("iret");
}

uint32_t pio_read(ioaddr_t, int);
void pio_write(ioaddr_t, int, uint32_t);

make_EHelper(in) {
  t0 = pio_read(id_src->val, id_src->width);
  operand_write(id_dest, &t0);

  print_asm_template2(in);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(out) {
  pio_write(id_dest->val, id_dest->width, id_src->val);

  print_asm_template2(out);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}
