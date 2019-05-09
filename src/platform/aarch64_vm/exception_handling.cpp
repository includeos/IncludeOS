#include <array>

#include <kprint>
#include "gic.h"

#include "exception_handling.hpp"
static std::array<irq_handler_t, 0x3ff> handlers;

void register_handler(uint16_t irq, irq_handler_t handler)
{
  handlers[irq]=handler;
}

void register_handler(uint16_t irq)
{
  handlers[irq]=0;
}

//is void correct ?
#if defined(__cplusplus)
extern "C" {
#endif

#include "cpu.h"

void exception_handler_irq_el(struct stack_frame *ctx,uint64_t esr)
{
  int irq=gicd_decode_irq();

  gicd_irq_disable(irq);
  //is this the role of the handler or the kernel ?
  gicd_irq_clear(irq);

  if (handlers[irq] != 0)
    handlers[irq]();

  gicd_irq_enable(irq);
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

const char * instruction_error(uint32_t code)
{
  switch(code&0x3F)
  {
    case 0b000000:
      return "Address size fault in TTBR0 or TTBR1.";
    case 0b000101:
      return "Translation fault, 1st level.";

    case 0b000110:
      return "Translation fault, 2nd level.";

    case 0b000111:
      return "Translation fault, 3rd level.";

    case 0b001001:
      return "Access flag fault, 1st level.";

    case 0b001010:
      return "Access flag fault, 2nd level.";

    case 0b001011:
      return "Access flag fault, 3rd level.";

    case 0b001101:
      return "Permission fault, 1st level.";

    case 0b001110:
      return "Permission fault, 2nd level.";

    case 0b001111:
      return "Permission fault, 3rd level.";

    case 0b010000:
      return "Synchronous external abort.";

    case 0b011000:
      return "Synchronous parity error on memory access.";

    case 0b010101:
      return "Synchronous external abort on translation table walk, 1st level.";

    case 0b010110:
      return "Synchronous external abort on translation table walk, 2nd level.";

    case 0b010111:
      return "Synchronous external abort on translation table walk, 3rd level.";

    case 0b011101:
      return "Synchronous parity error on memory access on translation table walk, 1st level.";

    case 0b011110:
      return "Synchronous parity error on memory access on translation table walk, 2nd level.";

    case 0b011111:
      return "Synchronous parity error on memory access on translation table walk, 3rd level.";

    case 0b100001:
      return "Alignment fault.";

    case 0b100010:
      return "Debug event.";
  }
  return "RESERVED";
}

void exception_handler_syn_el(struct stack_frame *ctx, uint64_t esr)
{
  kprintf("SYN EXCEPTION %08x\r\n",esr);
  uint8_t type = (esr>>26 )&0x3F;
  switch(type)
  {
    case 0x21:
      kprintf("Reason: %s\n",instruction_error(esr));
      _dump_regs(ctx,esr);
      break;
    //brk
    case 0x3C:
      kprintf("Reason: (BRK) code (%04x)\n",(((uint32_t)esr)&0xFFFF));
      _dump_regs(ctx,esr);
      break;
  }

  while(1);
}

void exception_handler_fiq_el(struct stack_frame *ctx,uint64_t esr)
{
//  kprintf("FIQ EXCEPTION el=%08x\r\n",cpu_get_current_el());
  kprintf("FIQ EXCEPTION\r\n");
}

void exception_handler_serror_el(struct stack_frame *ctx,uint64_t esr)
{
//  kprintf("SERROR EXCEPTION el=%08x\r\n",cpu_get_current_el());
  //kprint("SYN EXCEPTION\r\n");
  kprintf("SERROR EXCEPTION\r\n");
}

void exception_unhandled()
{
  kprintf("UNHANDLED EXCEPTION\r\n");
}
#if defined(__cplusplus)
}
#endif
