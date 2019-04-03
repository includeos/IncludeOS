#include <kprint>

#include "gic.h"

//is void correct ?
#if defined(__cplusplus)
extern "C" {
#endif

#include "cpu.h"

void exception_handler_irq_el(struct stack_frame *ctx,uint64_t esr)
{
//  kprintf("IRQ EXCEPTION el=%08x\r\n",cpu_get_current_el());
  //kprintf("IRQ EXCEPTION\r\n");

  int irq=gicd_decode_irq();
  kprintf("IRQ %d\r\n",irq);

  //TODO save processor irq state PSW
  //asm volatile("eret");
  gicd_irq_disable(irq);
  //is this the role of the handler or the kernel ?
  gicd_irq_clear(irq);
//  gicd_disable_int(irq);			/* Mask this irq */
//gic_v3_eoi(irq);				/* Send EOI for this irq line */
//timer_handler();
//gicd_enable_int(irq);			/* unmask this irq line */

}

struct stack_frame {
  uint64_t elr;
  uint64_t x[30];
};

static inline void _dump_regs(struct stack_frame *ctx,uint64_t esr)
{
  kprintf(" esr %016zx , ",esr);
  kprintf("elr %016zx\n",ctx->elr);
  for (int i=0;i<30;i+=2)
  {
    kprintf(" x%02d %016zx , ",i,ctx->x[i]);
    kprintf("x%02d %016zx\n",i+1,ctx->x[i+1]);
  }
  kprintf(" x%02d %016zx \n",30,ctx->x[30]);
}

void exception_handler_syn_el(struct stack_frame *ctx, uint64_t esr)
{
  kprintf("SYN EXCEPTION %08x\r\n",esr);
  uint8_t type = (esr>>26 )&0x3F;
  switch(type)
  {
    //brk
    case 0x3C:
      kprintf("Reason: (BRK) code (%04x)\n",(esr&0xFFFF));
      _dump_regs(ctx,esr);
      break;

  }
//  kprintf("SYN EXCEPTION el=%08x\r\n",cpu_get_current_el());
  //kprint("SYN EXCEPTION\r\n");
//  kprintf("SYN EXCEPTION %08x , %08x\r\n",ctx,syndrome);

/*
  0b000000
Address size fault in TTBR0 or TTBR1.

0b000101
Translation fault, 1st level.

00b00110
Translation fault, 2nd level.

00b00111
Translation fault, 3rd level.

0b001001
Access flag fault, 1st level.

0b001010
Access flag fault, 2nd level.

0b001011
Access flag fault, 3rd level.

0b001101
Permission fault, 1st level.

0b001110
Permission fault, 2nd level.

0b001111
Permission fault, 3rd level.

0b010000
Synchronous external abort.

0b011000
Synchronous parity error on memory access.

0b010101
Synchronous external abort on translation table walk, 1st level.

0b010110
Synchronous external abort on translation table walk, 2nd level.

0b010111
Synchronous external abort on translation table walk, 3rd level.

0b011101
Synchronous parity error on memory access on translation table walk, 1st level.

0b011110
Synchronous parity error on memory access on translation table walk, 2nd level.

0b011111
Synchronous parity error on memory access on translation table walk, 3rd level.

0b100001
Alignment fault.

0b100010
Debug event.
*/

  //asm volatile("eret");
  while(1);
}

void exception_handler_fiq_el(struct stack_frame *ctx,uint64_t esr)
{
//  kprintf("FIQ EXCEPTION el=%08x\r\n",cpu_get_current_el());
  kprintf("FIQ EXCEPTION\r\n");
  //asm volatile("eret");

}

void exception_handler_serror_el(struct stack_frame *ctx,uint64_t esr)
{
//  kprintf("SERROR EXCEPTION el=%08x\r\n",cpu_get_current_el());
  //kprint("SYN EXCEPTION\r\n");
  kprintf("SERROR EXCEPTION\r\n");
//  asm volatile("eret");

}

void exception_unhandled()
{
  kprintf("UNHANDLED EXCEPTION\r\n");
}
#if defined(__cplusplus)
}
#endif
