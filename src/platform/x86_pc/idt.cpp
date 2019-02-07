#include "idt.hpp"
#include <kernel/events.hpp>
#include <os.hpp>
#include <kprint>
#include <info>
#include <os>
#include <kernel/memory.hpp>
#define RING0_CODE_SEG   0x8

extern "C" {
  extern void unused_interrupt_handler();
  extern void modern_interrupt_handler();
  extern void spurious_intr();
  extern void cpu_enable_panicking();
}

#define IRQ_LINES  Events::NUM_EVENTS
#define INTR_LINES (IRQ_BASE + IRQ_LINES)

namespace x86
{
typedef void (*intr_handler_t)();
typedef void (*except_handler_t)();

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
  idt_entry->zero2     = 0;
#else
  (void) ist;
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

extern "C" {
  void __cpu_except_0();
  void __cpu_except_1();
  void __cpu_except_2();
  void __cpu_except_3();
  void __cpu_except_4();
  void __cpu_except_5();
  void __cpu_except_6();
  void __cpu_except_7();
  void __cpu_except_8();
  void __cpu_except_9();
  void __cpu_except_10();
  void __cpu_except_11();
  void __cpu_except_12();
  void __cpu_except_13();
  void __cpu_except_14();
  void __cpu_except_15();
  void __cpu_except_16();
  void __cpu_except_17();
  void __cpu_except_18();
  void __cpu_except_19();
  void __cpu_except_20();
  void __cpu_except_30();
}

void x86_IDT::init()
{
  // make sure its all zeroes
  memset(&this->entry[0], 0, sizeof(x86_IDT::entry));

  set_exception_handler(0, __cpu_except_0);
  set_exception_handler(1, __cpu_except_1);
  set_exception_handler(2, __cpu_except_2);
  set_exception_handler(3, __cpu_except_3);
  set_exception_handler(4, __cpu_except_4);
  set_exception_handler(5, __cpu_except_5);
  set_exception_handler(6, __cpu_except_6);
  set_exception_handler(7, __cpu_except_7);
  set_exception_handler(8, __cpu_except_8);
  set_exception_handler(9, __cpu_except_9);
  set_exception_handler(10, __cpu_except_10);
  set_exception_handler(11, __cpu_except_11);
  set_exception_handler(12, __cpu_except_12);
  set_exception_handler(13, __cpu_except_13);
  set_exception_handler(14, __cpu_except_14);
  set_exception_handler(15, __cpu_except_15);
  set_exception_handler(16, __cpu_except_16);
  set_exception_handler(17, __cpu_except_17);
  set_exception_handler(18, __cpu_except_18);
  set_exception_handler(19, __cpu_except_19);
  set_exception_handler(20, __cpu_except_20);
  set_exception_handler(30, __cpu_except_30);

  for (size_t i = 32; i < INTR_LINES - 2; i++) {
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

/// CPU EXCEPTIONS ///

#define PAGE_FAULT 14

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

void __cpu_dump_regs(uintptr_t* regs)
{
#if defined(ARCH_x86_64)
# define RIP_REG 16
  // AMD64 CPU registers
  struct desc_table_t {
    uint16_t  limit;
    uintptr_t location;
  } __attribute__((packed))  gdt, idt;
  asm ("sgdtq %0" : : "m" (* &gdt));
  asm ("sidtq %0" : : "m" (* &idt));

  fprintf(stderr, "\n");
  fprintf(stderr,"  RAX:  %016lx  R 8:  %016lx\n", regs[0], regs[5]);
  fprintf(stderr,"  RBX:  %016lx  R 9:  %016lx\n", regs[1], regs[6]);
  fprintf(stderr,"  RCX:  %016lx  R10:  %016lx\n", regs[2], regs[7]);
  fprintf(stderr,"  RDX:  %016lx  R11:  %016lx\n", regs[3], regs[8]);
  fprintf(stderr, "\n");

  fprintf(stderr,"  RBP:  %016lx  R12:  %016lx\n", regs[4], regs[9]);
  fprintf(stderr,"  RSP:  %016lx  R13:  %016lx\n", regs[13], regs[10]);
  fprintf(stderr,"  RSI:  %016lx  R14:  %016lx\n", regs[14], regs[11]);
  fprintf(stderr,"  RDI:  %016lx  R15:  %016lx\n", regs[15], regs[12]);
  fprintf(stderr,"  RIP:  %016lx  FLA:  %016lx\n", regs[16], regs[17]);
  fprintf(stderr, "\n");

  fprintf(stderr,"  CR0:  %016lx  CR4:  %016lx\n", regs[18], regs[22]);
  fprintf(stderr,"  CR1:  %016lx  CR8:  %016lx\n", regs[19], regs[23]);
  fprintf(stderr,"  CR2:  %016lx  GDT:  %016lx (%u)\n", regs[20], gdt.location, gdt.limit);
  fprintf(stderr,"  CR3:  %016lx  IDT:  %016lx (%u)\n", regs[21], idt.location, idt.limit);

#elif defined(ARCH_i686)
# define RIP_REG 8
  // i386 CPU registers
  fprintf(stderr, "\n");
  fprintf(stderr,"  EAX:  %08x  EBP:  %08x\n", regs[0], regs[4]);
  fprintf(stderr,"  EBX:  %08x  ESP:  %08x\n", regs[1], regs[5]);
  fprintf(stderr,"  ECX:  %08x  ESI:  %08x\n", regs[2], regs[6]);
  fprintf(stderr,"  EDX:  %08x  EDI:  %08x\n", regs[3], regs[7]);

  fprintf(stderr, "\n");
  fprintf(stderr,"  EIP:  %08x  EFL:  %08x\n", regs[8], regs[9]);

#else
  #error "Unknown architecture"
#endif
  fprintf(stderr, "\n");
}

extern "C" void double_fault(const char*);

void __page_fault(uintptr_t* regs, uint32_t code) {
  const char* reason = "Protection violation";
  auto addr = regs[20];

  if (not(code & 1))
    reason = "Page not present";

  fprintf(stderr,"%s, trying to access 0x%lx\n", reason, addr);

  if (code & 2)
    fprintf(stderr,"Page write failed.\n");
  else
    fprintf(stderr,"Page read failed.\n");


  if (code & 4)
    fprintf(stderr,"Privileged page access from user space.\n");

  if (code & 8)
    fprintf(stderr,"Found bit set in reserved field.\n");

  if (code & 16)
    fprintf(stderr,"Instruction fetch. XD\n");

  if (code & 32)
    fprintf(stderr,"Protection key violation.\n");

  if (code & 0x8000)
    fprintf(stderr,"SGX access violation.\n");

  auto key = os::mem::vmmap().in_range(addr);
  if (key) {
    auto& range = os::mem::vmmap().at(key);
    printf("Violated address is in mapped range \"%s\" \n", range.name());
  } else {
    printf("Violated address is outside mapped memory\n");
  }

}

static int exception_counter = 0;

extern "C"
__attribute__((noreturn, weak))
void __cpu_exception(uintptr_t* regs, int error, uint32_t code)
{
  __sync_fetch_and_add(&exception_counter, 1);
  if (exception_counter > 1) {
    os::panic("Double CPU exception");
  }

  SMP::global_lock();
  fprintf(stderr,"\n>>>> !!! CPU %u EXCEPTION !!! <<<<\n", SMP::cpu_id());
  fprintf(stderr,"%s (%d)   EIP  %p   CODE %#x\n",
          exception_names[error], error, (void*) regs[RIP_REG], code);

  if (error == PAGE_FAULT) {
    __page_fault(regs, code);
  }

  __cpu_dump_regs(regs);

  SMP::global_unlock();
  // error message:
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%s (%d)", exception_names[error], error);

  // normal CPU exception
  if (error != 0x8) {
    // call panic, which will decide what to do next
    os::panic(buffer);
  }
  else {
    // handle double faults differently
    double_fault(buffer);
  }

  __builtin_unreachable();
}
