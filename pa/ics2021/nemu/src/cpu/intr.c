#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */
  vaddr_t gate_addr = cpu.idtr.base + NO * 8;//乘8是因为IDT由一个8字节的描述符阵列组成
  GateDesc gateDesc;
  *(uint32_t *)&gateDesc = vaddr_read(gate_addr, 4);
  *((uint32_t *)&gateDesc + 1) = vaddr_read(gate_addr + 4, 4);
  uint32_t p = *((uint32_t *)&gateDesc + 1) & 0x8000;
  Assert(p != 0, "P is valid");
  rtl_push((rtlreg_t*)&cpu.eflags.value);
  rtl_push((rtlreg_t*)&cpu.cs);
  rtl_push((rtlreg_t*)&ret_addr);
  cpu.eflags.IF = 0;

  decoding.is_jmp = 1;
  decoding.jmp_eip = (gateDesc.offset_31_16 << 16) | (gateDesc.offset_15_0 & 0xffff);
}

void dev_raise_intr() {
  cpu.INTR = 1;
}
