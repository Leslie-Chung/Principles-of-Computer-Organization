#include "cpu/rtl.h"

/* Condition Code */

void rtl_setcc(rtlreg_t* dest, uint8_t subcode) { //不要用 t2
  bool invert = subcode & 0x1;
  enum {
    CC_O, CC_NO, CC_B,  CC_NB,
    CC_E, CC_NE, CC_BE, CC_NBE,
    CC_S, CC_NS, CC_P,  CC_NP,
    CC_L, CC_NL, CC_LE, CC_NLE
  };

  // TODO: Query EFLAGS to determine whether the condition code is satisfied.
  // dest <- ( cc is satisfied ? 1 : 0)
  switch (subcode & 0xe) {
    case CC_O: // OF == 1
      rtl_get_OF(&t0);
    	break;
    case CC_B: // CF == 1
      rtl_get_CF(&t0);
    	break;
    case CC_E: // ZF == 1
      rtl_get_ZF(&t0);
    	break;
    case CC_BE: // CF == 1 or ZF == 1 
    	rtl_get_CF(&t0);
    	rtl_get_ZF(&t1);
    	rtl_or(&t0, &t0, &t1);
    	break;
    case CC_S: // SF == 1 
    	rtl_get_SF(&t0);
    	break;
    case CC_L: // SF != OF
    	rtl_get_SF(&t0);
    	rtl_get_OF(&t1);
    	rtl_xor(&t0, &t0, &t1);
    	break;
    case CC_LE: // ZF == 1 or SF != OF
    	rtl_get_SF(&t0);
    	rtl_get_OF(&t1);
    	rtl_xor(&t0, &t0, &t1);	// s0 = (SF != OF)
    	rtl_get_ZF(&t1);		// s1 = (ZF == 1)
    	rtl_or(&t0, &t0, &t1);
    	break;
    default: panic("should not reach here");
    case CC_P: panic("n86 does not have PF");
  }
  *dest = t0;
  if (invert){
    rtl_xori(dest, dest, 0x1);
  }
}
