#pragma once
#include <cstdint>

namespace x86
{
  extern void init_libc(uint32_t magic, uint32_t address);
}

extern "C" {
  void kernel_sanity_checks();
  uintptr_t __syscall_entry();
}

#define LL_ASSERT(X) if (!(X)) { kprint("Early assertion failed: " #X "\n");  asm("cli;hlt"); }
