#include "tss.hpp"

#include <cstdio>
#include <malloc.h>
#include <smp>
#include <kernel/memory.hpp>

#ifdef DEBUG_IST
#define IST_PRINT(X, ...) printf(X, __VA_ARGS__)
#else
#define IST_PRINT(X, ...)
#endif

extern "C" void __amd64_load_tr(uint16_t);

struct gdtr64 {
  uint16_t limit;
  char*    location;
} __attribute__((packed));
extern gdtr64 __gdt64_base_pointer;

#define INTR_SIZE  4096 * 20
#define NMI_SIZE   4096 * 64
#define DFI_SIZE   4096 * 20
#define GUARD_SIZE 4096

struct stack {
  void* sp;
  void* phys;
};

static stack create_stack_virt(size_t size, const char* name)
{
  using namespace os;
  using namespace util::bitops;

  // Virtual memory area for CPU stacks.
  // TODO randomize location / ask virtual memory allocator
  const uintptr_t stack_area = 1ull << 46;

  const mem::Access flags = mem::Access::read | mem::Access::write;

  // Virtual area
  // Adds a guard page between each new stack
  static uintptr_t stacks_begin = stack_area + GUARD_SIZE;

  // Allocate physical memory
  auto* phys = (char*)memalign(4096, size);
  IST_PRINT("* Creating stack '%s' @ %p (%p phys) \n",
         name, (void*)stacks_begin, phys);

  const auto map = mem::map({stacks_begin, (uintptr_t)phys, flags, size}, name);

  Expects(map);
  Expects(mem::active_page_size(map.lin) == 4096);
  Expects(mem::flags(map.lin - 1) == mem::Access::none
          && "Guard page should not present");

  // Next stack starts after next page
  stacks_begin += util::bits::roundto<4096>(size) + GUARD_SIZE;

  // Align stack pointer to bottom of stack minus a pop
  auto sp = map.lin + size - 8;
  sp &= ~uintptr_t(0xf);

  // Force page fault if mapped area isn't writable
  ((char*)sp)[0] = '!';

  return {(void*) sp, phys};
}
static stack create_stack_simple(size_t size, const char* /*name*/)
{
  auto* phys = (char*)memalign(4096, size);
  uintptr_t sp = (uintptr_t) phys + size - 8;
  sp &= ~uintptr_t(0xf);
  return {(void*) sp, phys};
}

namespace x86
{
  struct alignas(SMP_ALIGN) LM_IST
  {
    void* intr;
    void* nmi;
    void* dfi;

    AMD64_TSS tss;
  };
  static std::array<LM_IST, SMP_MAX_CORES> lm_ist;

  void ist_initialize_for_cpu(int cpu, uintptr_t stack)
  {
    typedef struct stack (*create_stack_func_t) (size_t, const char*);
    create_stack_func_t create_stack = create_stack_virt;
    if (cpu > 0) create_stack = create_stack_simple;

    auto& ist = lm_ist.at(cpu);
    memset(&ist.tss, 0, sizeof(AMD64_TSS));

    auto st = create_stack(INTR_SIZE, "Intr stack");
    ist.tss.ist1 = (uintptr_t) st.sp;
    ist.intr = st.phys;

    st = create_stack(NMI_SIZE, "NMI stack");
    ist.tss.ist2 = (uintptr_t) st.sp;
    ist.nmi = st.phys;

    st = create_stack(DFI_SIZE, "DFI stack");
    ist.tss.ist3 = (uintptr_t) st.sp;
    ist.dfi = st.phys;

    ist.tss.rsp0 = stack;
    ist.tss.rsp1 = stack;
    ist.tss.rsp2 = stack;

    auto tss_addr = (uintptr_t) &ist.tss;

    // entry #3 in the GDT is the Task selector
    auto* tgd = (AMD64_TS*) &__gdt64_base_pointer.location[8 * 3];
    memset(tgd, 0, sizeof(AMD64_TS));
    tgd->td_type = 0x9;
    tgd->td_present = 1;
    tgd->td_lolimit = sizeof(AMD64_TSS) - 1;
    tgd->td_hilimit = 0;
    tgd->td_lobase  = tss_addr & 0xFFFFFF;
    tgd->td_hibase  = tss_addr >> 24;

    __amd64_load_tr(8 * 3);
  }
}
