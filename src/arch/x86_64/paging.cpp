// -*-C++-*-
// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 IncludeOS AS, Oslo, Norway
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
#include <arch/x86_paging.hpp>
#include <arch/x86_paging_utils.hpp>

//#define DEBUG_X86_PAGING
#ifdef DEBUG_X86_PAGING
#undef debug
#define debug(X, ...) printf("<x86 paging> " X "\n", ##__VA_ARGS__)
#endif
#define MYINFO(X,...) INFO("x86_64", X, ##__VA_ARGS__)


using namespace os::mem;
using namespace util;

using Flags = x86::paging::Flags;
using Pml4  = x86::paging::Pml4;

static void allow_executable();

/**
    IncludeOS default paging setup

    PML4 (512 * 512GB entries):
    - All 512 pointers allocated to prevent CPU reading garbage.
    - Entry 0 present, all other entries not present
    - Cost: ~1 page.

    PML3 (512 * 1GB entries):
    - 1 instances allocated (pointed to by PML4[0])
    - Entry 0 pointing to a PML2
    - All other entries identity mapped as present / writable / no_execute
    - Cost: ~1 page

    PML2 (512 * 2MB entries):
    - 1 instance allocated, 512 2MB entries covering the first GB
    - Entry 0 points to 4k page table for fine grained mapping of first 2MB
    - All other (1GB total) identity mapped as present / writable / no_execute
    - Cost: ~1 page

    PML1 (4k page table):
    - 1 instance for the first 2 MB (e.g. to protect the first 4k page)
    - instances as needed for covering the executable
    - Cost: 1 page + 1 page per 2 MB of executable

    Further instances dynamically allocated as needed, e.g. by calls to mem::map
**/

// The main page directory pointer
Pml4* __pml4;

void __arch_init_paging() {
  MYINFO("Initializing paging");
  __pml4 = x86::paging::allocate_pdir<Pml4>();
  Expects(__pml4 != nullptr);

  INFO2("* Adding 512 1GiB entries @ 0x0 -> 0x%llx", 512_GiB);
  auto* pml3_0 = __pml4->create_page_dir(0,Flags::present | Flags::writable |
                                           Flags::huge | Flags::no_exec);

  INFO2("* Adding 512 2MiB entries @ 0x0 -> 0x%llx", 1_GiB);
  pml3_0->create_page_dir(0,Flags::present | Flags::writable |
                            Flags::huge | Flags::no_exec);

  INFO2("* Marking page 0 as not present");
  __pml4->map_r({0, 0, Flags::none, 4_KiB, 4_KiB});

  INFO2("* Making .text segment executable");
  allow_executable();

  INFO2("* Passing page tables to CPU");
  extern void __x86_init_paging(void*);
  __x86_init_paging(__pml4->data());

}

void print_entry(uintptr_t ent)
{
  using namespace x86::paging;
  auto flags = __pml4->flags_of(ent);
  auto addr = __pml4->addr_of(ent);
  auto as_byte = Byte_r(addr);
  std::cout << as_byte << std::hex << " 0x" << ent
            << " Flags: " << flags << "\n";
}



namespace x86 {
namespace paging {

Access to_memflags(Flags f)
{

  Access prot = Access::none;

  if (! has_flag(f, Flags::present)) {
    prot |= Access::none;
    return prot;
  }

  prot |= Access::read;

  if (has_flag(f, Flags::writable)) {
    prot |= Access::write;
  }

  if (! has_flag(f, Flags::no_exec)) {
    prot |= Access::execute;
  }

  return prot;
}

Flags to_x86(os::mem::Access prot)
{
  Flags flags = Flags::none;
  if (prot != Access::none) {
    flags |= Flags::present;
  }

  if (has_flag(prot, Access::write)) {
    flags |= Flags::writable;
  }

  if (not has_flag(prot, Access::execute)) {
    flags |= Flags::no_exec;
  }

  return flags;
}

void invalidate(void *pageaddr){
  asm volatile("invlpg (%0)" ::"r" (pageaddr) : "memory");
}

}
}


namespace os {
namespace mem {

using Map_x86 = Mapping<x86::paging::Flags>;

Map to_mmap(Map_x86 map){
  return {map.lin, map.phys, to_memflags(map.flags), map.size, map.page_size};
}

Map_x86 to_x86(Map map){
  return {map.lin, map.phys, x86::paging::to_x86(map.flags), map.size, map.page_size};
}

Access protect_page(uintptr_t linear, Access flags)
{
  debug("::protect 0x%lx, size %li \n", linear, size);
  x86::paging::Flags xflags = x86::paging::to_x86(flags);
  auto f = __pml4->set_flags_r(linear, xflags);
  x86::paging::invalidate((void*)linear);
  return to_memflags(f);
};

Access protect(uintptr_t linear, Access flags)
{
  debug("::protect 0x%lx, size %li \n", linear, size);
  x86::paging::Flags xflags = x86::paging::to_x86(flags);

  auto key = OS::memory_map().in_range(lin);
  // Throws if entry wasn't previously mapped.
  auto map_ent = OS::memory_map().at(key);

  int sz_prot = 0;
  x86::paging::Flags fl;

  while (sz_prot < map_ent.size()){
    auto page = linear + sz_prot;
    auto psize = active_page_size(page);
    fl |= __pml4->set_flags_r(page, xflags);
    sz_prot += page_size;
    x86::paging::invalidate((void*)linear);
  }
  return to_memflags(fl);
};

Access flags(uintptr_t addr)
{
  return to_memflags(__pml4->flags_r(addr));
}

Map map(uintptr_t linear, size_t size, Access flags)
{
  Expects(linear != 0);
  using namespace x86::paging;
  void* phys_ = nullptr;
  if (! posix_memalign(&phys_, 4_KiB, size))
    return Map();

  auto phys = reinterpret_cast<uintptr_t>(phys_);
  auto res = __pml4->map_r({linear, phys, x86::paging::to_x86(flags), size});
  return to_mmap(res);
}

Map map(Map m, const char* name)
{
  Expects(m.size > 0);

  // Disallow mapping the 0-page
  Expects(m.lin != 0 and m.phys != 0);

  if (strcmp(name, "") == 0)
    name = "mem::map";

  OS::memory_map().assign_range({m.lin, m.lin + m.size - 1, name});
  using namespace x86::paging;

  auto new_map = __pml4->map_r(to_x86(m));
  if (new_map) {
    debug("Wants : %s size: %li", m.to_string().c_str(), m.size);
    debug("Gets  : %s size: %li", new_map.to_string().c_str(), new_map.size);

    // Size should match requested size rounded up to nearest 4k
    Ensures(new_map.size == roundto<4_KiB>(m.size));

    // Flags should contain all expected flags, possibly including read
    Ensures(has_flag(to_memflags(new_map.flags), m.flags));
    Ensures(is_aligned<4_KiB>(new_map.page_size));
  }

  return to_mmap(new_map);
};

Map unmap(uintptr_t lin){
  auto key = OS::memory_map().in_range(lin);
  Map_x86 m;
  if (key) {
    auto map_ent = OS::memory_map().at(key);
    m.lin = lin;
    m.phys = 0;
    m.size = map_ent.size();

    m = __pml4->map_r({key, 0, x86::paging::to_x86(Access::none), (size_t)map_ent.size()});

    Ensures(m.size == roundto<4_KiB>(map_ent.size()));
    OS::memory_map().erase(key);
  }

  return to_mmap(m);
}

uintptr_t active_page_size(uintptr_t addr){
  return __pml4->active_page_size(addr);
}

uintptr_t active_page_size(void* addr){
  return __pml4->active_page_size(reinterpret_cast<uintptr_t>(addr));
}

}
}


extern char _TEXT_START_;
extern char _TEXT_END_;
uintptr_t __exec_begin = (uintptr_t)&_TEXT_START_;
uintptr_t __exec_end = (uintptr_t)&_TEXT_END_;

void allow_executable()
{
  Expects(is_aligned<4_KiB>(__exec_begin));
  Expects(__exec_end > __exec_begin);

  auto exec_size = __exec_end - __exec_begin;
  auto page_size = exec_size >= 2_MiB ? 2_MiB : 4_KiB;

  os::mem::Map m;
  m.lin       = __exec_begin;
  m.phys      = __exec_begin;
  m.size      = exec_size;
  m.page_size = page_size;
  m.flags     = os::mem::Access::execute | os::mem::Access::read;

  os::mem::map(m, "ELF .text");
}
