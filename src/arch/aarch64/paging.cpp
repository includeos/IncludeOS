// -*-C++-*-

#include <arch.hpp>
#include <kernel/memory.hpp>

__attribute__((weak))
void __arch_init_paging()
{
  INFO("aarch64", "Paging not enabled by default on");
}

namespace os {
namespace mem {
  __attribute__((weak))
  Map map(Map m, const char* name) {
    return {};
  }

  template <>
  const size_t Mapping<os::mem::Access>::any_size = 4096;
}
}
