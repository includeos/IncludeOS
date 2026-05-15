// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <os>
#include <kernel.hpp>
#include <kprint>
#include <boot/multiboot.h>
#include <kernel/memory.hpp>
#include <fmt/format.h>
#include <ranges>

template<class... Args>
static inline void _kfmt(fmt::string_view prefix, fmt::format_string<Args...> fmtstr, Args&&... args) {
  fmt::basic_memory_buffer<char, kernel::kprintf_max_size> buf;
  fmt::format_to_n(std::back_inserter(buf), buf.capacity(), "{}", prefix);
  fmt::format_to_n(std::back_inserter(buf), buf.capacity() - buf.size(), fmtstr, std::forward<Args>(args)...);

  kprintf("%.*s", (int)buf.size(), buf.data());
}
#define DEBUG_MULTIBOOT
#if defined(DEBUG_MULTIBOOT)
#undef debug
#define debug(fmt, ...)  _kfmt("", fmt, ##__VA_ARGS__)
#undef MYINFO
#define MYINFO(fmt, ...) _kfmt("< multiboot >", fmt "\n", ##__VA_ARGS__)
#undef INFO2
#define INFO2(fmt, ...) _kfmt("\t", fmt "\n", ##__VA_ARGS__)
#else
#define debug(...)  ((void) 0)
#define MYINFO(fmt, ,...) INFO("Kernel", X, ##__VA_ARGS__)
#endif

extern uintptr_t _end;


using namespace util::bitops;
using namespace util::literals;

static inline multiboot_info_t* bootinfo(uint32_t addr)
{
  // NOTE: the address is 32-bit and not a pointer
  return (multiboot_info_t*) (uintptr_t) addr;
}
#if defined(ARCH_aarch64)
  uint32_t dummy[24];
  uintptr_t __multiboot_addr=(uintptr_t)&dummy[0];
#else
  extern uint32_t __multiboot_addr;
#endif

multiboot_info_t* kernel::bootinfo()
{
  return (multiboot_info_t*) (uintptr_t) __multiboot_addr;
}

uintptr_t _multiboot_memory_end(uintptr_t boot_addr) {
  auto* info = bootinfo(boot_addr);
  if (info->flags & MULTIBOOT_INFO_MEMORY) {
    return 0x100000 + (info->mem_upper * 1024);
  }
  return os::Arch::max_canonical_addr;
}

// Deterimine the end of multiboot provided data
// (e.g. multiboot's data area as offset to the _end symbol)
uintptr_t _multiboot_free_begin(uintptr_t boot_addr)
{
  auto* info = bootinfo(boot_addr);
  uintptr_t multi_end = reinterpret_cast<uintptr_t>(&_end);

  debug("* Multiboot begin: {:#x}\n", (uintptr_t)info);
  if (info->flags & MULTIBOOT_INFO_CMDLINE
      and info->cmdline > multi_end)
  {
    debug("* Multiboot cmdline @ 0x{:08x}: {}\n", info->cmdline, reinterpret_cast<char*>(info->cmdline));
    // We can't use a cmdline that's either insde our ELF or pre-ELF area
    Expects(info->cmdline > multi_end
            or info->cmdline < 0x100000);

    if (info->cmdline > multi_end) {
      auto* cmdline_ptr = (const char*) (uintptr_t) info->cmdline;
      // Set free begin to after the cmdline string
      multi_end = info->cmdline + strlen(cmdline_ptr) + 1;
    }
  }

  debug("* Multiboot end: {:#x}\n", multi_end);
  if (info->mods_count == 0)
      return multi_end;

  auto* mods_list = (multiboot_module_t*) (uintptr_t) info->mods_addr;
  debug("* Module list @ {}\n", static_cast<const void*>(mods_list));

  for (multiboot_module_t* mod = mods_list;
       mod < mods_list + info->mods_count;
       mod ++) {

    debug("\t * Module @ {:#x}\n", mod->mod_start);
    debug("\t * Args: {}\n ", (char*) (uintptr_t) mod->cmdline);
    debug("\t * End: {:#x}\n", mod->mod_end);

    if (mod->mod_end > multi_end)
      multi_end = mod->mod_end;

  }

  debug("* Multiboot end: {} \n", multi_end);
  return multi_end;
}

constexpr static inline const char* multiboot_memory_type_str(uint32_t type) noexcept {
  // TODO: convert multiboot types to enum class
  switch (type) {
    case MULTIBOOT_MEMORY_AVAILABLE:
      return "Available";
    case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
      return "ACPI Reclaimable";
    case MULTIBOOT_MEMORY_NVS:
      return "ACPI Non-volatile Storage";
    case MULTIBOOT_MEMORY_BADRAM:
      return "Bad RAM";
    case MULTIBOOT_MEMORY_RESERVED:
      return "Reserved";
    default:
        return "UNKNOWN";
  }
}


std::span<multiboot_memory_map_t> _multiboot_memory_maps() {
  auto* info = kernel::bootinfo();

  auto* hardware_map = reinterpret_cast<multiboot_memory_map_t*>(info->mmap_addr);
  const size_t entry_count = static_cast<size_t>(info->mmap_length / sizeof(multiboot_memory_map_t));

  return std::span<multiboot_memory_map_t> { hardware_map, entry_count };
}

void kernel::multiboot(uint32_t boot_addr)
{
#if defined(__x86_64)
  MYINFO("Booted with multiboot x86_64");
#else
  MYINFO("Booted with multiboot x86");
#endif
  auto* info = ::bootinfo(boot_addr);
  INFO2("* Boot flags: {:#x}", info->flags);

  if (info->flags & MULTIBOOT_INFO_MEMORY) {
    uint32_t mem_low_start = 0;
    uint32_t mem_low_end = (info->mem_lower * 1024) - 1;
    uint32_t mem_low_kb = info->mem_lower;
    uint32_t mem_high_start = 0x100000;
    uint32_t mem_high_end = mem_high_start + (info->mem_upper * 1024) - 1;
    uint32_t mem_high_kb = info->mem_upper;

    INFO2("* Valid memory ({} KiB):", mem_low_kb + mem_high_kb);
    INFO2("  0x{:08x} - 0x{:08x} ({} KiB)", mem_low_start, mem_low_end, mem_low_kb);
    INFO2("  0x{:08x} - 0x{:08x} ({} KiB)", mem_high_start, mem_high_end, mem_high_kb);
    INFO2("");
  }
  else {
    INFO2("No memory information from multiboot");
  }

  if (info->flags & MULTIBOOT_INFO_CMDLINE) {
    const auto* cmdline = (const char*) (uintptr_t) info->cmdline;
    INFO2("* Booted with parameters @ {}: {}", (const void*)(uintptr_t)info->cmdline, cmdline);
    kernel::state().cmdline = std::pmr::string(cmdline).data();
  }

  if (info->flags & MULTIBOOT_INFO_MEM_MAP) {
    auto* hardware_map = reinterpret_cast<multiboot_memory_map_t*>(info->mmap_addr);
    const size_t entry_count = static_cast<size_t>(info->mmap_length / sizeof(multiboot_memory_map_t));

    INFO2("* Multiboot provided memory map  ({} entries @ {})\n", entry_count, reinterpret_cast<const void*>(hardware_map));

    for (auto map : std::span<multiboot_memory_map_t>{ hardware_map, entry_count })
    {
      const uintptr_t start = map.addr;
      const uintptr_t size = map.len;
      const uintptr_t end = start + size - 1;

      INFO2("  {:#16x} - {:#16x} ({} KiB): {}", start, end, size / 1024, multiboot_memory_type_str(map.type));

      // os::mem::map() does not accept non-aligned page addresses
      if (not util::bits::is_aligned<4_KiB>(map.addr)) {
        os::mem::vmmap().assign_range({start, start + size - 1, "UNALIGNED"});
        continue;
      }

      os::mem::Map rw_map = { /*.linear=*/start, /*.physical=*/start, /*.fl=*/os::mem::Access::read | os::mem::Access::write, /*.sz=*/size };
      switch (map.type)
      {
        case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
            os::mem::map(rw_map, "Multiboot (ACPI Reclaimable)");
            break;
        case MULTIBOOT_MEMORY_NVS:
            os::mem::map(rw_map, "Multiboot (ACPI Non-volatile Storage)");
            break;
        case MULTIBOOT_MEMORY_BADRAM:
            os::mem::map(rw_map, "Multiboot (Bad RAM)");
            break;
        case MULTIBOOT_MEMORY_RESERVED:
            os::mem::map(rw_map, "Multiboot (Reserved)");
            break;

        case MULTIBOOT_MEMORY_AVAILABLE: {
          // these are mapped in src/platform/${platform}/os.cpp
          break;
        }
        default: {
          char buf[32];  // libc is not entirely initialized at this point
          std::snprintf(buf, sizeof(buf), "Unknown memory map type: %d", map.type);
          os::panic(buf);
        }
      }
    }
    INFO2("");
  }

  auto mods = os::modules();

  if (not mods.empty()) {
    MYINFO("OS loaded with {} modules", mods.size());
    for (auto mod : mods) {
      INFO2("* {} @ 0x{:08x} - 0x{:08x}, size: {} B",
            reinterpret_cast<char*>(mod.params),
            mod.mod_start, mod.mod_end, mod.mod_end - mod.mod_start);
    }
  }
}
