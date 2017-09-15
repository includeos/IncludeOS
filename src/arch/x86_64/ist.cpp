#include "tss.hpp"
#include <cstdio>
#include <smp>
extern "C" void __amd64_load_tr(uint16_t);

struct gdtr64 {
  uint16_t limit;
  char*    location;
} __attribute__((packed));
extern gdtr64 __gdt64_base_pointer;

#define INTR_SIZE  4096
#define NMI_SIZE  20480
#define DFI_SIZE   4096

namespace x86
{
  struct alignas(SMP_ALIGN) LM_IST
  {
    char* intr; // 4kb
    char* nmi;  // 20kb
    char* dfi;  // 4kb

    AMD64_TSS tss;
  };
  static std::array<LM_IST, SMP_MAX_CORES> lm_ist;

  void ist_initialize_for_cpu(int cpu, uintptr_t stack)
  {
    auto& ist = lm_ist.at(cpu);
    ist.intr = new char[INTR_SIZE];
    ist.nmi  = new char[NMI_SIZE];
    ist.dfi  = new char[DFI_SIZE];

    memset(&ist.tss, 0, sizeof(AMD64_TSS));
    ist.tss.rsp0 = stack;
    ist.tss.rsp1 = stack;
    ist.tss.rsp2 = stack;

    ist.tss.ist1 = (uintptr_t) ist.intr + INTR_SIZE - 16;
    ist.tss.ist2 = (uintptr_t) ist.nmi  + NMI_SIZE - 16;
    ist.tss.ist3 = (uintptr_t) ist.dfi  + DFI_SIZE - 16;

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
