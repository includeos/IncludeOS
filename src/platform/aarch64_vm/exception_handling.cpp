#include <kprint>

#include "gic.h"

//is void correct ?
#if defined(__cplusplus)
extern "C" {
#endif

#include "cpu.h"

void exception_handler_irq_el()
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

void exception_handler_syn_el(uint64_t a, uint64_t b)
{
//  kprintf("SYN EXCEPTION el=%08x\r\n",cpu_get_current_el());
  //kprint("SYN EXCEPTION\r\n");
  kprintf("SYN EXCEPTION %08x , %08x\r\n",a,b);
  //asm volatile("eret");

}

void exception_handler_fiq_el()
{
//  kprintf("FIQ EXCEPTION el=%08x\r\n",cpu_get_current_el());
  kprintf("FIQ EXCEPTION\r\n");
  //asm volatile("eret");

}

void exception_handler_serror_el()
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
