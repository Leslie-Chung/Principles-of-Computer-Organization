#include "cpu/exec.h"

make_EHelper(add)
{
  rtl_add(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);

  rtl_sltu(&t0, &t2, &id_dest->val);
  rtl_set_CF(&t0);

  rtl_xor(&t0, &id_dest->val, &id_src->val);
  rtl_not(&t0);
  rtl_xor(&t1, &id_dest->val, &t2);
  rtl_and(&t0, &t0, &t1);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);
}

make_EHelper(sub)
{
  if (id_src->width == 1 && id_dest->width >= 2)
  {
    rtl_sext(&id_src->val, &id_src->val, id_src->width);
  }

  rtl_sub(&t2, &id_dest->val, &id_src->val); // t2 = dest->val - src->val
  rtl_sltu(&t3, &id_dest->val, &t2);         // t3 = dest->val < dest->val - src->val 正常情况下是0，如果借位为1
  operand_write(id_dest, &t2);               // dest->reg = t2 或 dest->mem = t2

  rtl_update_ZFSF(&t2, id_dest->width); //更新ZF 和 SF

  rtl_set_CF(&t3); // 判断是否有借位

  //减法时，两个数的符号相异才可能溢出
  rtl_xor(&t0, &id_dest->val, &id_src->val); // t0 = dest->val ^ src->val，判断dest和src最高位是否相异，相异为1
  rtl_xor(&t1, &id_dest->val, &t2);          // t1 = dest->val ^ (dest->val - src->val)，判断dest和结果最高位是否相异，相异为1
  rtl_and(&t0, &t0, &t1);                    // t0 = t0 & t1
  rtl_msb(&t0, &t0, id_dest->width);         // 获取t0的最高有效位（8*width - 1）
  rtl_set_OF(&t0);                           // 判断是否溢出

  print_asm_template2(sub);
}

make_EHelper(cmp)
{
  rtl_sext(&id_src->val, &id_src->val, id_src->width);
  rtl_sub(&t2, &id_dest->val, &id_src->val); // t2 = dest->val - src->val
  rtl_sltu(&t3, &id_dest->val, &t2);         // t3 = dest->val < dest->val - src->val 正常情况下是0，如果借位为1
  rtl_update_ZFSF(&t2, id_dest->width);      //更新ZF 和 SF

  rtl_set_CF(&t3); // 判断是否有借位

  //减法时，两个数的符号相异才可能溢出
  rtl_xor(&t0, &id_dest->val, &id_src->val); // t0 = dest->val ^ src->val，判断dest和src最高位是否相异，相异为1
  rtl_xor(&t1, &id_dest->val, &t2);          // t1 = dest->val ^ (dest->val - src->val)，判断dest和结果最高位是否相异，相异为1
  rtl_and(&t0, &t0, &t1);                    // t0 = t0 & t1
  rtl_msb(&t0, &t0, id_dest->width);         // 获取t0的最高有效位（8*width - 1）
  rtl_set_OF(&t0);                           // 判断是否溢出

  print_asm_template2(cmp);
}

make_EHelper(inc)
{
  t3 = 1;
  rtl_add(&t2, &id_dest->val, &t3);
  rtl_sltu(&t0, &t2, &id_dest->val); // dest + src < dest，没有进位则应该是0
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);
  rtl_set_CF(&t0);

  rtl_xor(&t0, &id_dest->val, &t3); // dest ^ src，判断最高位是否相异，相异为1
  rtl_not(&t0);                     // ~(dest ^ src)，如果最高位相异，则取反后为0
  rtl_xor(&t1, &id_dest->val, &t2); // dest ^ (dest + src) 判断dest和结果最高位是否相异，相异为1
  rtl_and(&t0, &t0, &t1);           // ~(dest ^ src) & (dest ^ (dest + src))，如果最高位相同且dest与结果的最高位相异，则溢出
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template1(inc);
}

make_EHelper(dec)
{

  t3 = 1;
  rtl_sub(&t2, &id_dest->val, &t3);  // t2 = dest->val - src->val
  rtl_sltu(&t3, &id_dest->val, &t2); // t3 = dest->val < dest->val - src->val 正常情况下是0，如果借位为1
  operand_write(id_dest, &t2);       // dest->reg = t2 或 dest->mem = t2

  rtl_update_ZFSF(&t2, id_dest->width); //更新ZF 和 SF

  rtl_set_CF(&t3); // 判断是否有借位

  t3 = 1;
  //减法时，两个数的符号相异才可能溢出
  rtl_xor(&t0, &id_dest->val, &t3);  // t0 = dest->val ^ src->val，判断dest和src最高位是否相异，相异为1
  rtl_xor(&t1, &id_dest->val, &t2);  // t1 = dest->val ^ (dest->val - src->val)，判断dest和结果最高位是否相异，相异为1
  rtl_and(&t0, &t0, &t1);            // t0 = t0 & t1
  rtl_msb(&t0, &t0, id_dest->width); // 获取t0的最高有效位（8*width - 1）
  rtl_set_OF(&t0);                   // 判断是否溢出

  print_asm_template1(dec);
}

make_EHelper(neg)
{
  t0 = id_dest->val != 0;
  rtl_set_CF(&t0);
  t0 = -id_dest->val;
  rtl_update_ZFSF(&t0, id_dest->width);
  operand_write(id_dest, &t0);

  t0 = 0;
  if ((1 << (8 * id_dest->width - 1)) == id_dest->val)
  {
    t0 = 1;
  }
  rtl_set_OF(&t0);

  print_asm_template1(neg);
}

make_EHelper(adc)
{
  rtl_add(&t2, &id_dest->val, &id_src->val); // t2 = dest + src
  rtl_sltu(&t3, &t2, &id_dest->val);         // t3 = t2 < dest 如果溢出t3 = 1
  rtl_get_CF(&t1);
  rtl_add(&t2, &t2, &t1);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);

  rtl_sltu(&t0, &t2, &id_dest->val);
  rtl_or(&t0, &t3, &t0);
  rtl_set_CF(&t0);

  rtl_xor(&t0, &id_dest->val, &id_src->val);
  rtl_not(&t0);

  rtl_xor(&t1, &id_dest->val, &t2);
  rtl_and(&t0, &t0, &t1);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template2(adc);
}

make_EHelper(sbb)
{
  /* DEST ← DEST - (SRC + CF) 

  从第一个操作数（SRC）中减去这个结果。减法的结果被分配给第一个操作数（DEST），并相应地设置标志。
  
  受影响的标志 OF、SF、ZF和CF
  */
  rtl_sub(&t2, &id_dest->val, &id_src->val); // t2 = dest->val - src->val
  rtl_sltu(&t3, &id_dest->val, &t2);         // t3 = dest->val < dest->val - src->val 正常情况下是0，如果借位为1
  rtl_get_CF(&t1);                           // 获取低位的借位
  rtl_sub(&t2, &t2, &t1);                    // t2 = dest->val - src->val - CF
  operand_write(id_dest, &t2);               // dest->reg = t2 或 dest->mem = t2

  rtl_update_ZFSF(&t2, id_dest->width); //更新ZF 和 SF

  rtl_sltu(&t0, &id_dest->val, &t2); // t0 = dest->val < dest->val - src->val - CF 正常情况下是0，如果借位为1
  rtl_or(&t0, &t3, &t0);             // t0 = t0 | t3
  rtl_set_CF(&t0);                   // 判断是否有借位

  //减法时，两个数的符号相异才可能溢出
  rtl_xor(&t0, &id_dest->val, &id_src->val); // t0 = dest->val ^ src->val，判断dest和src最高位是否相异，相异为1
  rtl_xor(&t1, &id_dest->val, &t2);          // t1 = dest->val ^ (dest->val - src->val - CF)，判断dest和结果最高位是否相异，相异为1
  rtl_and(&t0, &t0, &t1);                    // t0 = t0 & t1
  rtl_msb(&t0, &t0, id_dest->width);         // 获取t0的最高有效位（8*width - 1）
  rtl_set_OF(&t0);                           // 判断是否溢出

  print_asm_template2(sbb);
}

make_EHelper(mul)
{
  rtl_lr(&t0, R_EAX, id_dest->width);
  rtl_mul(&t0, &t1, &id_dest->val, &t0);

  switch (id_dest->width)
  {
  case 1:
    rtl_sr_w(R_AX, &t1);
    break;
  case 2:
    rtl_sr_w(R_AX, &t1);
    rtl_shri(&t1, &t1, 16);
    rtl_sr_w(R_DX, &t1);
    break;
  case 4:
    rtl_sr_l(R_EDX, &t0);
    rtl_sr_l(R_EAX, &t1);
    break;
  default:
    assert(0);
  }

  print_asm_template1(mul);
}

// imul with one operand
make_EHelper(imul1)
{
  rtl_lr(&t0, R_EAX, id_dest->width);
  rtl_imul(&t0, &t1, &id_dest->val, &t0);

  switch (id_dest->width)
  {
  case 1:
    rtl_sr_w(R_AX, &t1);
    break;
  case 2:
    rtl_sr_w(R_AX, &t1);
    rtl_shri(&t1, &t1, 16);
    rtl_sr_w(R_DX, &t1);
    break;
  case 4:
    rtl_sr_l(R_EDX, &t0);
    rtl_sr_l(R_EAX, &t1);
    break;
  default:
    assert(0);
  }

  print_asm_template1(imul);
}

// imul with two operands
make_EHelper(imul2)
{
  rtl_sext(&id_src->val, &id_src->val, id_src->width);
  rtl_sext(&id_dest->val, &id_dest->val, id_dest->width);

  rtl_imul(&t0, &t1, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t1);

  print_asm_template2(imul);
}

// imul with three operands
make_EHelper(imul3)
{
  rtl_sext(&id_src->val, &id_src->val, id_src->width);
  rtl_sext(&id_src2->val, &id_src2->val, id_src->width);
  rtl_sext(&id_dest->val, &id_dest->val, id_dest->width);

  rtl_imul(&t0, &t1, &id_src2->val, &id_src->val);
  operand_write(id_dest, &t1);

  print_asm_template3(imul);
}

make_EHelper(div)
{
  switch (id_dest->width)
  {
  case 1:
    rtl_li(&t1, 0);
    rtl_lr_w(&t0, R_AX);
    break;
  case 2:
    rtl_lr_w(&t0, R_AX);
    rtl_lr_w(&t1, R_DX);
    rtl_shli(&t1, &t1, 16);
    rtl_or(&t0, &t0, &t1);
    rtl_li(&t1, 0);
    break;
  case 4:
    rtl_lr_l(&t0, R_EAX);
    rtl_lr_l(&t1, R_EDX);
    break;
  default:
    assert(0);
  }

  rtl_div(&t2, &t3, &t1, &t0, &id_dest->val);

  rtl_sr(R_EAX, id_dest->width, &t2);
  if (id_dest->width == 1)
  {
    rtl_sr_b(R_AH, &t3);
  }
  else
  {
    rtl_sr(R_EDX, id_dest->width, &t3);
  }

  print_asm_template1(div);
}

make_EHelper(idiv)
{
  rtl_sext(&id_dest->val, &id_dest->val, id_dest->width);

  switch (id_dest->width)
  {
  case 1:
    rtl_lr_w(&t0, R_AX);
    rtl_sext(&t0, &t0, 2);
    rtl_msb(&t1, &t0, 4);
    rtl_sub(&t1, &tzero, &t1);
    break;
  case 2:
    rtl_lr_w(&t0, R_AX);
    rtl_lr_w(&t1, R_DX);
    rtl_shli(&t1, &t1, 16);
    rtl_or(&t0, &t0, &t1);
    rtl_msb(&t1, &t0, 4);
    rtl_sub(&t1, &tzero, &t1);
    break;
  case 4:
    rtl_lr_l(&t0, R_EAX);
    rtl_lr_l(&t1, R_EDX);
    break;
  default:
    assert(0);
  }

  rtl_idiv(&t2, &t3, &t1, &t0, &id_dest->val);

  rtl_sr(R_EAX, id_dest->width, &t2);
  if (id_dest->width == 1)
  {
    rtl_sr_b(R_AH, &t3);
  }
  else
  {
    rtl_sr(R_EDX, id_dest->width, &t3);
  }

  print_asm_template1(idiv);
}
