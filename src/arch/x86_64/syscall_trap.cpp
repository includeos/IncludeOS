#include <arch/x86/cpu.hpp>
#include <arch/x86/syscall.hpp>

extern "C" void __syscall_entry();

namespace x86 {

void init_syscall_trap()
{
  CPU::write_msr(IA32_STAR, (8ull << 32) | (8ull << 48));
  CPU::write_msr(IA32_LSTAR, (uintptr_t)&__syscall_entry);
}

}
