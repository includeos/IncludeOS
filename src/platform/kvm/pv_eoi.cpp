#include <kprint>
#include <cstdint>
#include <arch/x86/cpu.hpp>
#include "../x86_pc/apic.hpp"
#include <info>
#include <smp>

// current selected EOI method
extern void (*current_eoi_mechanism)();
extern void (*real_eoi_mechanism)();
extern void (*current_intr_handler)();

// *** manual ***
// http://choon.net/forum/read.php?21,1123399
// https://www.kernel.org/doc/Documentation/virtual/kvm/cpuid.txt

#define KVM_MSR_ENABLED        1
#define MSR_KVM_PV_EOI_EN      0x4b564d04
#define KVM_PV_EOI_BIT         0
#define KVM_PV_EOI_MASK       (0x1 << KVM_PV_EOI_BIT)
#define KVM_PV_EOI_ENABLED     KVM_PV_EOI_MASK

struct alignas(SMP_ALIGN) pv_eoi
{
  uint32_t eoi_word = 0;
};
static SMP::Array<pv_eoi> exitless_eoi;

extern "C"
void kvm_pv_eoi()
{
  uint8_t reg;
  asm("btr %2, %0; setc %1" : "+m"(PER_CPU(exitless_eoi).eoi_word), "=rm"(reg) : "r"(0));
  if (reg) {
      kprintf("avoidable eoi\n");
      return;
  }
  else {
    //kprintf("APIC IRR: %d\n", x86::APIC::get_irr());
  }
  // fallback to normal x2APIC EOI
  real_eoi_mechanism();
}
void kvm_pv_eoi_init()
{
  uint64_t addr = (uint64_t) &PER_CPU(exitless_eoi).eoi_word;
  addr |= KVM_MSR_ENABLED;
  x86::CPU::write_msr(MSR_KVM_PV_EOI_EN, addr);
  // verify that the feature was enabled
  uint64_t res = x86::CPU::read_msr(MSR_KVM_PV_EOI_EN);
  if (res & 1) {
    INFO("KVM", "Paravirtual EOI enabled");
    // set new EOI handler
    current_eoi_mechanism = kvm_pv_eoi;
  }
}
