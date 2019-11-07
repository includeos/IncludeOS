
#pragma once
#include <cstdint>
#include <string>
#include <arch.hpp>

namespace x86
{
  struct SMBIOS
  {
    static void init();

    static inline
    const arch_system_info_t& system_info() {
      return sysinfo;
    }

  private:
    static void parse(const char*);
    static arch_system_info_t sysinfo;
  };
}
