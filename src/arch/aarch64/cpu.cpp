#include <stdint.h>
#include <kprint>

#define DAIF_DBG_BIT      (1<<3)
#define DAIF_ABT_BIT      (1<<2)
#define DAIF_IRQ_BIT      (1<<1)
#define DAIF_FIQ_BIT      (1<<0)

#if defined(__cplusplus)
extern "C" {
#endif

#define CURRENT_EL_MASK   0x3
#define CURRENT_EL_SHIFT  2
uint32_t cpu_get_current_el()
{
  uint32_t el;
  asm volatile("mrs %0, CurrentEL": "=r" (el)::);
  return ((el>>CURRENT_EL_SHIFT)&CURRENT_EL_MASK);
}

void cpu_print_current_el()
{
  uint32_t el=cpu_get_current_el();
  kprintf("CurrentEL %08x\r\n",el);
}

void cpu_fiq_enable()
{
  asm volatile("msr DAIFClr,%0" ::"i"(DAIF_FIQ_BIT): "memory");
}

void cpu_irq_enable()
{
  asm volatile("msr DAIFClr,%0" ::"i"(DAIF_IRQ_BIT) : "memory");
}

void cpu_serror_enable()
{
  asm volatile("msr DAIFClr,%0" ::"i"(DAIF_ABT_BIT) : "memory");
}

void cpu_debug_enable()
{
  asm volatile("msr DAIFClr,%0" ::"i"(DAIF_DBG_BIT): "memory");
}

void cpu_fiq_disable()
{
  asm volatile("msr DAIFSet,%0" ::"i"(DAIF_FIQ_BIT): "memory");
}

void cpu_irq_disable()
{
  asm volatile("msr DAIFSet,%0" ::"i"(DAIF_IRQ_BIT) : "memory");
}

void cpu_serror_disable()
{
  asm volatile("msr DAIFSet,%0" ::"i"(DAIF_ABT_BIT) : "memory");
}

void cpu_debug_disable()
{
  asm volatile("msr DAIFSet,%0" ::"i"(DAIF_DBG_BIT): "memory");
}

void cpu_disable_all_exceptions()
{
  asm volatile("msr DAIFSet,%0" :: "i"(DAIF_FIQ_BIT|DAIF_IRQ_BIT|DAIF_ABT_BIT|DAIF_DBG_BIT) : "memory");
}

void cpu_wfi()
{
  asm volatile("wfi" : : : "memory");
}

void cpu_disable_exceptions(uint32_t irq)
{
  //seting mask to 0 enables the irq
  //__asm__ __volatile__("msr DAIFClr, %0\n\t" :: "r"(irq) : "memory");
  //OLD WAY
  volatile uint32_t daif;
  asm volatile("mrs %0 , DAIF" : "=r"(daif)::"memory");
  daif &=~irq;
  asm volatile("msr DAIF, %0 " : :"r"(daif):"memory");
}

void cpu_enable_exceptions(uint32_t irq)
{
  //setting the mask to 1 disables the irq
  //asm volatile("msr daifset, %0\n\t" :: "r"(irq):"memory");
  //OLD WAY

  volatile uint32_t daif;
  asm volatile("mrs %0 , DAIF" : "=r"(daif)::"memory");
  daif |=irq;
  asm volatile("msr DAIF, %0" : :"r"(daif):"memory");
}

#if defined(__cplusplus)
}
#endif
