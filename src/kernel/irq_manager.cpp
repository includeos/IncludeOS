// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//#define DEBUG // Enable debugging
//#define DEBUG2

#include <assert.h>

#include <os>
#include <hw/pic.hpp>
#include <kernel/irq_manager.hpp>
#include <kernel/syscalls.hpp>
#include <unwind.h>


unsigned int IRQ_manager::irq_mask  {0xFFFB};
IDTDescr     IRQ_manager::idt[irq_lines]  {};
bool IRQ_manager::idt_is_set        {false};

irq_bitfield IRQ_manager::irq_pending_ {0};
irq_bitfield IRQ_manager::irq_subscriptions_ {0};

void (*IRQ_manager::irq_subscribers_[sizeof(irq_bitfield)*8])() {nullptr};
IRQ_manager::irq_delegate IRQ_manager::irq_delegates_[sizeof(irq_bitfield)*8];

void IRQ_manager::enable_interrupts() {
  __asm__ volatile("sti");
}

inline void disable_pic();

enum {
  CPUID_FEAT_EDX_APIC = 1 << 9
};

static inline void cpuid(int code, uint32_t *a, uint32_t *d) {
  asm volatile("cpuid"
               :"=a"(*a),"=d"(*d)
               :"a"(code)
               :"ecx","ebx");
}

bool cpuHasAPIC() {
  uint32_t eax, edx;
  cpuid(1, &eax, &edx);
  return edx & CPUID_FEAT_EDX_APIC;
}

extern "C"
{
  void exception_handler() __attribute__((noreturn));
}

/** Default Exception-handler, which just prints its number */
#define EXCEPTION_HANDLER(I)                                    \
  void exception_##I##_handler() {                              \
    printf("\n\n>>>> !!! CPU EXCEPTION %i !!! <<<<<\n", I);     \
    printf("Heap end: %#x \n", (uint32_t)&_end);                \
    kill(1, 9);                                                 \
  }

void exception_handler()
{
#define frp(N, ra)                                      \
  (__builtin_frame_address(N) != nullptr) &&            \
    (ra = __builtin_return_address(N)) != nullptr

  printf("\n");
#define PRINT_TRACE(N, ra)                      \
  printf("[%d] Return %p\n", N, ra);

  void* ra;
  if (frp(0, ra)) {
    PRINT_TRACE(0, ra);
    if (frp(1, ra)) {
      PRINT_TRACE(1, ra);
      if (frp(2, ra)) {
        PRINT_TRACE(2, ra);
        if (frp(3, ra)) {
          PRINT_TRACE(3, ra);
          if (frp(4, ra)) {
            PRINT_TRACE(4, ra);
            if (frp(5, ra)) {
              PRINT_TRACE(5, ra);
              if (frp(6, ra))
                PRINT_TRACE(6, ra);
            }}}}}}

  printf(">>>> !!! CPU EXCEPTION !!! <<<<\n");
  extern char _end;
  printf("Heap end: %#x \n", (uint32_t) &_end);
  panic(">>>> !!! CPU EXCEPTION !!! <<<<\n");
}

/**
 *  Default IRQ Handler
 *
 *  - Set pending flag
 *  - Increment counter
 */
uint32_t IRQ_manager::irq_counters_[32] {0};


#define IRQ_HANDLER(I)                          \
  void irq_##I##_handler() {                    \
    IRQ_manager::register_interrupt(I);         \
  }

/* Macro magic to register default gates */
#define REG_DEFAULT_EXCPT(I) create_gate(&(idt[I]),exception_entry,     \
                                         default_sel, default_attr );

#define REG_DEFAULT_IRQ(I) create_gate(&(idt[I + irq_base]),irq_##I##_entry, \
                                       default_sel, default_attr );

/* EXCEPTIONS */
#define EXCEPTION_PAIR(I) void exception_entry();
#define IRQ_PAIR(I) void irq_##I##_entry(); IRQ_HANDLER(I)

/*
  IRQ HANDLERS,
  extern: must be visible from assembler

  We define two functions for each IRQ/Exception i :

  > void irq/exception_i_entry() - defined in interrupts.s
  > void irq/exception_i_handler() - defined here.
*/
extern "C"{
  void _irq_20_entry(int i);

  void irq_default_handler();
  void irq_default_entry();

  void irq_timer_entry();
  void irq_timer_handler();

  // CPU-sampling irq-handler is defined in the PIT-implementation
  void cpu_sampling_irq_entry();

  EXCEPTION_PAIR(0) EXCEPTION_PAIR(1) EXCEPTION_PAIR(2) EXCEPTION_PAIR(3)
  EXCEPTION_PAIR(4) EXCEPTION_PAIR(5) EXCEPTION_PAIR(6) EXCEPTION_PAIR(7)
  EXCEPTION_PAIR(8) EXCEPTION_PAIR(9) EXCEPTION_PAIR(10) EXCEPTION_PAIR(11)
  EXCEPTION_PAIR(12) /*EXCEPTION_PAIR(13)*/ EXCEPTION_PAIR(14) EXCEPTION_PAIR(15)
  EXCEPTION_PAIR(16) EXCEPTION_PAIR(17) EXCEPTION_PAIR(18) EXCEPTION_PAIR(19)
  EXCEPTION_PAIR(20) /*21-29 Reserved*/ EXCEPTION_PAIR(30) EXCEPTION_PAIR(31)

  void exception_13_entry();
  void exception_13_handler(int i){
    printf("\n>>>>!!!! CPU Exception 13 !!!! \n");
    printf("\t >> General protection fault. Error code: 0x%x \n",i);
    if (i == 0)
      printf("\t >> Error code is 0, so not segment related \n");
    printf("\t >> Stack address: %p\n", (void*) &i);
    kill(1,9);
  }

  //Redirected IRQ 0 - 15
  IRQ_PAIR(0) IRQ_PAIR(1) IRQ_PAIR(3) IRQ_PAIR(4) IRQ_PAIR(5)
  IRQ_PAIR(6) IRQ_PAIR(7) IRQ_PAIR(8) IRQ_PAIR(9) IRQ_PAIR(10)
  IRQ_PAIR(11) IRQ_PAIR(12) IRQ_PAIR(13) IRQ_PAIR(14) IRQ_PAIR(15)

} //End extern

void IRQ_manager::init()
{
  //debug("CPU HAS APIC: %s \n", cpuHasAPIC() ? "YES" : "NO" );
  if (idt_is_set)
    panic(">>> ERROR: Trying to reset IDT");

  //Create an idt entry for the 'lidt' instruction
  idt_loc idt_reg;
  idt_reg.limit = (irq_lines*sizeof(IDTDescr))-1;
  idt_reg.base = (uint32_t)idt;

  INFO("IRQ manager", "Creating interrupt handlers");

  // Assign the lower 32 IRQ's : Exceptions
  REG_DEFAULT_EXCPT(0) REG_DEFAULT_EXCPT(1) REG_DEFAULT_EXCPT(2)
    REG_DEFAULT_EXCPT(3) REG_DEFAULT_EXCPT(4) REG_DEFAULT_EXCPT(5)
    REG_DEFAULT_EXCPT(6) REG_DEFAULT_EXCPT(7) REG_DEFAULT_EXCPT(8)
    REG_DEFAULT_EXCPT(9) REG_DEFAULT_EXCPT(10) REG_DEFAULT_EXCPT(11)
    REG_DEFAULT_EXCPT(12) REG_DEFAULT_EXCPT(13) REG_DEFAULT_EXCPT(14)
    REG_DEFAULT_EXCPT(15) REG_DEFAULT_EXCPT(16) REG_DEFAULT_EXCPT(17)
    REG_DEFAULT_EXCPT(18) REG_DEFAULT_EXCPT(19) REG_DEFAULT_EXCPT(20)
    // GATES 21-29 are reserved
    REG_DEFAULT_EXCPT(30) REG_DEFAULT_EXCPT(31)
    INFO2("+ Exception gates set for irq < 32");

  //Redirected IRQ 0 - 15
  REG_DEFAULT_IRQ(0) REG_DEFAULT_IRQ(1) REG_DEFAULT_IRQ(3)
    REG_DEFAULT_IRQ(4) REG_DEFAULT_IRQ(5) REG_DEFAULT_IRQ(6)
    REG_DEFAULT_IRQ(7) REG_DEFAULT_IRQ(8) REG_DEFAULT_IRQ(9)
    REG_DEFAULT_IRQ(10) REG_DEFAULT_IRQ(11) REG_DEFAULT_IRQ(12)
    REG_DEFAULT_IRQ(13) REG_DEFAULT_IRQ(14) REG_DEFAULT_IRQ(15)

    //Set all irq-gates (> 47) to the default handler
    for(int i=48;i<irq_lines;i++){
      create_gate(&(idt[i]),irq_default_entry,default_sel,default_attr);
    }
  INFO2("+ Default interrupt gates set for irq >= 32");

  //Load IDT
  __asm__ volatile ("lidt %0": :"m"(idt_reg) );

  //Initialize the interrupt controller
  //hw::PIC::init();
  enable_interrupts();
}

// A union to be able to extract the lower and upper part of an address
union addr_union {
  uint32_t whole;
  struct {
    uint16_t lo16;
    uint16_t hi16;
  };
};

void IRQ_manager::create_gate(IDTDescr* idt_entry,
                              void (*function_addr)(),
                              uint16_t segment_sel,
                              char attributes) {
  addr_union addr;
  addr.whole           = (uint32_t)function_addr;
  idt_entry->offset_1  = addr.lo16;
  idt_entry->offset_2  = addr.hi16;
  idt_entry->selector  = segment_sel; //TODO: Create memory vars. Private OS-class?
  idt_entry->type_attr = attributes;
  idt_entry->zero      = 0;
}

void IRQ_manager::set_handler(uint8_t irq, void(*function_addr)()) {
  create_gate(&idt[irq], function_addr, default_sel, default_attr);

  /**
   *  The default handlers don't send EOI. If we don't do it here,
   *  previous interrupts won't have reported EOI and new handler
   *  will never get called
   */
  eoi(irq - irq_base);
}

void (*IRQ_manager::get_handler(uint8_t irq))() {
  addr_union addr;
  addr.lo16 = idt[irq].offset_1;
  addr.hi16 = idt[irq].offset_2;

  return (void (*)()) addr.whole;
}

IRQ_manager::irq_delegate IRQ_manager::get_subscriber(uint8_t irq) {
  return irq_delegates_[irq];
}

void IRQ_manager::enable_irq(uint8_t irq) {
  hw::PIC::enable_irq(irq);
}

int IRQ_manager::timer_interrupts {0};
static int glob_timer_interrupts  {0};

/** Let's say we only use 32 IRQ-lines. Then we can use a simple uint32_t
    as bitfield for setting / checking IRQ's. */
void IRQ_manager::subscribe(uint8_t irq, irq_delegate del) {   //void(*notify)()
  if (irq > (sizeof(irq_bitfield) * 8))
    panic("Too high IRQ: only IRQ 0 - 32 are subscribable\n");

  // Enable the IRQ line
  enable_irq(irq);

  // Mark IRQ as subscribed to
  irq_subscriptions_ |= (1 << irq);

  // Add callback to subscriber list (for now overwriting any previous)
  //irq_subscribers[irq] = notify;
  irq_delegates_[irq] = del;

  eoi(irq);
  INFO("IRQ manager", "Updated subscriptions: %#x irq: %i", irq_subscriptions_, irq);
}

/** Get most significant bit of b. */
inline int bsr(irq_bitfield b) {
  int ret {0};
  __asm__ volatile("bsr %1,%0":"=r"(ret):"r"(b));
  return ret;
}

void IRQ_manager::notify() {
  //__asm__("cli");

  // Get the IRQ's that are both pending and subscribed to
  irq_bitfield todo {static_cast<irq_bitfield>(irq_subscriptions_ & irq_pending_)};
  int          irq  {0};

  while (todo) {

    // Select the first IRQ to notify - the least significant bit set
    // - lowesr bit/IRQ, means higher priority
    irq = __builtin_ffs(todo) - 1;

    // Notify
    debug2("<IRQ notify> __irqueue %i Count: %i\n", irq, irq_counters_[irq]);
    irq_delegates_[irq]();

    // Decrement the counter
    __sync_fetch_and_sub(&irq_counters_[irq], 1);

    // Critical section start
    // Spinlock? Well, we can't lock out the IRQ-handler
    // ... and we don't have a timer interrupt so we can't do blocking locks.
    if (!irq_counters_[irq]) {
      // Remove the IRQ from pending list
      irq_pending_ &= ~(1 << irq);
      //debug("<IRQ notify> IRQ's pending: 0x%lx\n",irq_pending_);
    }
    // Critical section end

    // Find remaining IRQ's both pending and subscribed to
    todo = (irq_subscriptions_ & irq_pending_);
  }

  //hlt
  debug2("<IRQ notify> Done. OS going to sleep.\n");
  //__asm__("sti");
  __asm__ volatile("hlt;");
}

void IRQ_manager::eoi(uint8_t irq) {
  hw::PIC::eoi(irq);
}

void irq_default_handler() {
  // Now we don't really know the IRQ number,
  // but we can guess by looking at ISR
  uint16_t isr {hw::PIC::get_isr()};

  //IRR would give us more than we want
  //uint16_t irr {pic_get_irr()};

  printf("\n <IRQ !!!> Unexpected IRQ. ISR: 0x%x. EOI: 0x%x\n", isr, bsr(isr));
  IRQ_manager::eoi(bsr(isr));
}

void irq_timer_handler() {
  ++glob_timer_interrupts;
  if((glob_timer_interrupts % 16) == 0) {
    printf("\nGot %i timer interrupts\n", glob_timer_interrupts);
  }
}

inline void disable_pic() {
  asm volatile("mov $0xff,%al; "                \
               "out %al,$0xa1; "                      \
               "out %al,$0x21; ");
}
