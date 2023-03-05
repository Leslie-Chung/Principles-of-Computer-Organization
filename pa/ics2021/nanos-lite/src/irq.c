#include "common.h"
_RegSet *do_syscall(_RegSet *r);
_RegSet *schedule(_RegSet *prev);
static _RegSet *do_event(_Event e, _RegSet *r){
  _RegSet *ret = NULL;
  switch (e.event) {
    case _EVENT_TRAP:
      return schedule(r);
      break;
    case _EVENT_SYSCALL:
      return do_syscall(r);
    case _EVENT_IRQ_TIME:
      ret = schedule(r);
      //Log("Time_IRQ");
      break;
    default: panic("Unhandled event ID = %d", e.event);
  }

  return ret;
}

void init_irq(void) {
  _asye_init(do_event);
}
