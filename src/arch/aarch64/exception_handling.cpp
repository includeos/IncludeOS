#include <kprint>


//is void correct ?
#if defined(__cplusplus)
extern "C" {
#endif

#include "cpu.h"

void exception_handler_irq_el()
{
//  kprintf("IRQ EXCEPTION el=%08x\r\n",cpu_get_current_el());
kprint("IRQ EXCEPTION\r\n");
  asm volatile("eret");
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
  kprint("FIQ EXCEPTION\r\n");
  asm volatile("eret");

}

void exception_handler_serror_el()
{
//  kprintf("SERROR EXCEPTION el=%08x\r\n",cpu_get_current_el());
  //kprint("SYN EXCEPTION\r\n");
  kprint("SERROR EXCEPTION\r\n");
  asm volatile("eret");

}
#if defined(__cplusplus)
}
#endif
