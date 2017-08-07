#include "idt.hpp"
#include <kernel/irq_manager.hpp>
#include <kernel/syscalls.hpp>
#include <kprint>
#include <info>
#define RING0_CODE_SEG   0x8

extern "C" {
  extern void unused_interrupt_handler();
  extern void modern_interrupt_handler();
  extern void spurious_intr();
}

#define INTR_LINES IRQ_manager::INTR_LINES
#define IRQ_LINES  IRQ_manager::IRQ_LINES

namespace x86
{
typedef void (*intr_handler_t)();
typedef void (*except_handler_t)(void**, uint32_t);

struct x86_IDT
{
  IDTDescr entry[INTR_LINES] __attribute__((aligned(16)));

  intr_handler_t get_handler(uint8_t vec);
  void set_handler(uint8_t, intr_handler_t);
  void set_exception_handler(uint8_t, except_handler_t);

  void init();
};
static std::array<x86_IDT, SMP_MAX_CORES> idt;

void idt_initialize_for_cpu(int cpu) {
  idt.at(cpu).init();
}

/// CPU EXCEPTIONS ///

static const char* exception_names[] =
{
  "Divide-by-zero Error",
  "Debug",
  "Non-maskable Interrupt",
  "Breakpoint",
  "Overflow",
  "Bound Range Exceeded",
  "Invalid Opcode",
  "Device Not Available",
  "Double Fault",
  "Reserved",
  "Invalid TSS",
  "Segment Not Present",
  "Stack-Segment Fault",
  "General Protection Fault",
  "Page Fault",
  "Reserved",
  "x87 Floating-point Exception",
  "Alignment Check",
  "Machine Check",
  "SIMD Floating-point Exception",
  "Virtualization Exception",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Security Exception",
  "Reserved"
};

// Only certain exceptions have error codes
template <int NR>
typename std::enable_if<((NR >= 8 and NR <= 15) or NR == 17 or NR == 30)>::type
print_error_code(uint32_t err){
  kprintf("Error code: 0x%x \n", err);
}

// Default: no error code
template <int NR>
typename std::enable_if<((NR < 8 or NR > 15) and NR != 17 and NR != 30)>::type
print_error_code(uint32_t){}

inline void cpu_dump_regs()
{
#if defined(ARCH_x86_64)
  // CPU registers
  uintptr_t regs[24];
  asm ("movq %%rax, %0" : "=a" (regs[0]));
  asm ("movq %%rbx, %0" : "=b" (regs[1]));
  asm ("movq %%rcx, %0" : "=c" (regs[2]));
  asm ("movq %%rdx, %0" : "=d" (regs[3]));
  asm ("movq %%rbp, %0" : "=a" (regs[4]));

  asm ("movq %%r8, %0"  : "=a" (regs[5]));
  asm ("movq %%r9, %0"  : "=b" (regs[6]));
  asm ("movq %%r10, %0" : "=c" (regs[7]));
  asm ("movq %%r11, %0" : "=d" (regs[8]));
  asm ("movq %%r12, %0" : "=a" (regs[9]));
  asm ("movq %%r13, %0" : "=b" (regs[10]));
  asm ("movq %%r14, %0" : "=c" (regs[11]));
  asm ("movq %%r15, %0" : "=d" (regs[12]));

  asm ("movq %%rsp, %0" : "=a" (regs[13]));
  asm ("movq %%rsi, %0" : "=b" (regs[14]));
  asm ("movq %%rdi, %0" : "=c" (regs[15]));
  asm ("leaq (%%rip), %0" : "=d" (regs[16]));

  asm ("pushf; popq %0" : "=a" (regs[17]));
  asm ("movq %%cr0, %0" : "=b" (regs[18]));
  regs[19] = 0;
  asm ("movq %%cr2, %0" : "=c" (regs[20]));
  asm ("movq %%cr3, %0" : "=d" (regs[21]));
  asm ("movq %%cr4, %0" : "=a" (regs[22]));
  asm ("movq %%cr8, %0" : "=b" (regs[23]));

  struct desc_table_t {
    uint16_t  limit;
    uintptr_t location;
  } __attribute__((packed))  gdt, idt;
  asm ("sgdtq %0" : : "m" (* &gdt));
  asm ("sidtq %0" : : "m" (* &idt));

  fprintf(stderr, "\n");
  printf("  RAX:  %016lx  R 8:  %016lx\n", regs[0], regs[5]);
  printf("  RBX:  %016lx  R 9:  %016lx\n", regs[1], regs[6]);
  printf("  RCX:  %016lx  R10:  %016lx\n", regs[2], regs[7]);
  printf("  RDX:  %016lx  R11:  %016lx\n", regs[3], regs[8]);
  fprintf(stderr, "\n");

  printf("  RBP:  %016lx  R12:  %016lx\n", regs[4], regs[9]);
  printf("  RSP:  %016lx  R13:  %016lx\n", regs[13], regs[10]);
  printf("  RSI:  %016lx  R14:  %016lx\n", regs[14], regs[11]);
  printf("  RDI:  %016lx  R15:  %016lx\n", regs[15], regs[12]);
  printf("  RIP:  %016lx  FLA:  %016lx\n", regs[16], regs[17]);
  fprintf(stderr, "\n");

  printf("  CR0:  %016lx  CR4:  %016lx\n", regs[18], regs[22]);
  printf("  CR1:  %016lx  CR8:  %016lx\n", regs[19], regs[23]);
  printf("  CR2:  %016lx  GDT:  %016lx (%u)\n", regs[20], gdt.location, gdt.limit);
  printf("  CR3:  %016lx  IDT:  %016lx (%u)\n", regs[21], idt.location, idt.limit);

#elif defined(ARCH_i686)
  // CPU registers
  uintptr_t regs[16];
  asm ("movl %%eax, %0" : "=a" (regs[0]));
  asm ("movl %%ebx, %0" : "=b" (regs[1]));
  asm ("movl %%ecx, %0" : "=c" (regs[2]));
  asm ("movl %%edx, %0" : "=d" (regs[3]));

  asm ("movl %%ebp, %0" : "=r" (regs[4]));
  asm ("movl %%esp, %0" : "=r" (regs[5]));
  asm ("movl %%esi, %0" : "=r" (regs[6]));
  asm ("movl %%edi, %0" : "=r" (regs[7]));

  asm ("movl (%%esp), %0" : "=r" (regs[8]));
  asm ("pushf; popl %0" : "=r" (regs[9]));

  fprintf(stderr, "\n");
  printf("  EAX:  %08x  EBP:  %08x\n", regs[0], regs[4]);
  printf("  EBX:  %08x  ESP:  %08x\n", regs[1], regs[5]);
  printf("  ECX:  %08x  ESI:  %08x\n", regs[2], regs[6]);
  printf("  EDX:  %08x  EDI:  %08x\n", regs[3], regs[7]);
  printf("  EIP:  %08x  EFL:  %08x\n", regs[8], regs[9]);

#else
  #error "Unknown architecture"
#endif
  fprintf(stderr, "\n");
}

template <int NR>
void cpu_exception(void** eip, uint32_t error)
{
  SMP::global_lock();
  cpu_dump_regs();
  kprintf("\n>>>> !!! CPU %u EXCEPTION !!! <<<<\n", SMP::cpu_id());
  kprintf("    %s (%d)   EIP  %p\n", exception_names[NR], NR, eip);
  print_error_code<NR>(error);
  SMP::global_unlock();
  // call panic, which will decide what to do next
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%s (%d)", exception_names[NR], NR);
  panic(buffer);
}

// A union to be able to extract the lower and upper part of an address
union addr_union {
  uintptr_t whole;
  struct {
    uint16_t lo16;
    uint16_t hi16;
#ifdef ARCH_x86_64
    uint32_t top32;
#endif
  };
};

static void set_intr_entry(
    IDTDescr* idt_entry,
    intr_handler_t func,
    uint8_t  ist,
    uint16_t segment_sel,
    char attributes)
{
  addr_union addr;
  addr.whole           = (uintptr_t) func;
  idt_entry->offset_1  = addr.lo16;
  idt_entry->offset_2  = addr.hi16;
#ifdef ARCH_x86_64
  idt_entry->offset_3  = addr.top32;
#endif
  idt_entry->selector  = segment_sel;
  idt_entry->type_attr = attributes;
#ifdef ARCH_x86_64
  idt_entry->ist       = ist;
#else
  idt_entry->zero      = 0;
#endif
}

intr_handler_t x86_IDT::get_handler(uint8_t vec)
{
  addr_union addr;
  addr.lo16  = entry[vec].offset_1;
  addr.hi16  = entry[vec].offset_2;
#ifdef ARCH_x86_64
  addr.top32 = entry[vec].offset_3;
#endif

  return (intr_handler_t) addr.whole;
}

void x86_IDT::set_handler(uint8_t vec, intr_handler_t func) {
  set_intr_entry(&entry[vec], func, 1, RING0_CODE_SEG, 0x8e);
}
void x86_IDT::set_exception_handler(uint8_t vec, except_handler_t func) {
  set_intr_entry(&entry[vec], (intr_handler_t) func, 2, RING0_CODE_SEG, 0x8e);
}

void x86_IDT::init()
{
  if (SMP::cpu_id() == 0)
      INFO("INTR", "Creating exception handlers");
  set_exception_handler(0, cpu_exception<0>);
  set_exception_handler(1, cpu_exception<1>);
  set_exception_handler(2, cpu_exception<2>);
  set_exception_handler(3, cpu_exception<3>);
  set_exception_handler(4, cpu_exception<4>);
  set_exception_handler(5, cpu_exception<5>);
  set_exception_handler(6, cpu_exception<6>);
  set_exception_handler(7, cpu_exception<7>);
  set_exception_handler(8, cpu_exception<8>);
  set_exception_handler(9, cpu_exception<9>);
  set_exception_handler(10, cpu_exception<10>);
  set_exception_handler(11, cpu_exception<11>);
  set_exception_handler(12, cpu_exception<12>);
  set_exception_handler(13, cpu_exception<13>);
  set_exception_handler(14, cpu_exception<14>);
  set_exception_handler(15, cpu_exception<15>);
  set_exception_handler(16, cpu_exception<16>);
  set_exception_handler(17, cpu_exception<17>);
  set_exception_handler(18, cpu_exception<18>);
  set_exception_handler(19, cpu_exception<19>);
  set_exception_handler(20, cpu_exception<20>);
  set_exception_handler(30, cpu_exception<30>);

  if (SMP::cpu_id() == 0)
      INFO2("+ Default interrupt gates set for irq >= 32");

  for (size_t i = 32; i < INTR_LINES - 1; i++) {
    set_handler(i, unused_interrupt_handler);
  }
  // spurious interrupt handler
  set_handler(INTR_LINES - 1, spurious_intr);

  // Load IDT
  IDTR idt_reg;
  idt_reg.limit = INTR_LINES * sizeof(IDTDescr) - 1;
  idt_reg.base = (uintptr_t) &entry[0];
  asm volatile ("lidt %0" : : "m"(idt_reg));
}

} // x86

void __arch_subscribe_irq(uint8_t irq)
{
  assert(irq < IRQ_LINES);
  PER_CPU(x86::idt).set_handler(IRQ_BASE + irq, modern_interrupt_handler);
}
void __arch_install_irq(uint8_t irq, x86::intr_handler_t handler)
{
  assert(irq < IRQ_LINES);
  PER_CPU(x86::idt).set_handler(IRQ_BASE + irq, handler);
}
void __arch_unsubscribe_irq(uint8_t irq)
{
  assert(irq < IRQ_LINES);
  PER_CPU(x86::idt).set_handler(IRQ_BASE + irq, unused_interrupt_handler);
}
