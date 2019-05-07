#pragma once
#include <cstdint>

namespace aarch64
{
  extern void init_libc(uintptr_t dtb);
}

extern "C" {
  void kernel_sanity_checks();
  uintptr_t __syscall_entry();
}

#define LL_ASSERT(X) if (!(X)) { kprint("Early assertion failed: " #X "\n");  asm("hlt 0xf000"); }
