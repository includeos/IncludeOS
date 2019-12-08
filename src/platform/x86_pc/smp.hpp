
#pragma once
#ifndef X86_SMP_HPP
#define X86_SMP_HPP

#include <cstdint>
#include <vector>
#include <smp>

namespace x86 {

extern void init_SMP();
extern void initialize_gdt_for_cpu(int cpuid);

}

#endif
