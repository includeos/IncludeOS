
#pragma once
#ifndef X86_SMP_HPP
#define X86_SMP_HPP

#include <cstdint>
#include <vector>
#include <smp>

typedef SMP::task_func smp_task_func;
typedef SMP::done_func smp_done_func;

namespace x86 {

extern void init_SMP();
extern void initialize_gdt_for_cpu(int cpuid);

}

#endif
