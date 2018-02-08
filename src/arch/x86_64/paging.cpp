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
#include <kernel/cpuid.hpp>

template<>
const size_t  x86::paging::Map::any_size { supported_page_sizes() };

template<>
const size_t  os::mem::Map::any_size { supported_page_sizes() };

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

__attribute__((weak))
void __arch_init_paging() {
  INFO("x86_64", "Initializing paging");
  __pml4 = x86::paging::allocate_pdir<Pml4>();
  Expects(__pml4 != nullptr);

  INFO2("* Supported page sizes: %s", os::mem::page_sizes_str(os::mem::Map::any_size).c_str());

  auto default_fl = Flags::present | Flags::writable | Flags::huge | Flags::no_exec;
  INFO2("* Adding 512 1GiB entries @ 0x0 -> 0x%llx", 512_GiB);
  /* pml3_0 = */ __pml4->create_page_dir(0, default_fl);

  if (not os::mem::supported_page_size(1_GiB)) {
    auto first_range = __pml4->map_r({0,0,default_fl, 16_GiB});
    Expects(first_range && first_range.size >= 16_GiB);
    INFO2("* Identity mapping %s", first_range.to_string().c_str());
  }

  INFO2("* Marking page 0 as not present");
  auto zero_page = __pml4->map_r({0, 0, Flags::none, 4_KiB, 4_KiB});

  allow_executable();

  Expects(zero_page.size == 4_KiB);
  Expects(zero_page.page_sizes == 4_KiB);
  Expects(__pml4->active_page_size(0LU) == 4_KiB);

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

__attribute__((weak))
void invalidate(void *pageaddr){
  asm volatile("invlpg (%0)" ::"r" (pageaddr) : "memory");
}

}} // x86::paging

namespace os {
namespace mem {

using Map_x86 = Mapping<x86::paging::Flags>;

#define mem_fail_fast(reason)                                               \
  throw Memory_exception(std::string(__FILE__) + ":" + std::to_string(__LINE__) \
                         + ": " + reason)


uintptr_t supported_page_sizes() {
  static size_t res = 0;
  if (res == 0) {
    res = 4_KiB | 2_MiB;
    if (CPUID::has_feature(CPUID::Feature::PDPE1GB))
      res |= 1_GiB;
  }
  return  res;
}

uintptr_t min_psize()
{
  return bits::keepfirst(supported_page_sizes());
}

uintptr_t max_psize()
{
  return bits::keeplast(supported_page_sizes());
}

bool supported_page_size(uintptr_t size)
{
  return bits::is_pow2(size) and (size & supported_page_sizes()) != 0;
}

Map to_mmap(Map_x86 map){
  return {map.lin, map.phys, to_memflags(map.flags), map.size, map.page_sizes};
}

Map_x86 to_x86(Map map){
  return {map.lin, map.phys, x86::paging::to_x86(map.flags), map.size, map.page_sizes};
}

Access protect_page(uintptr_t linear, Access flags)
{
  PG_PRINT("::protect_page 0x%lx\n", linear);
  x86::paging::Flags xflags = x86::paging::to_x86(flags);
  auto f = __pml4->set_flags_r(linear, xflags);
  x86::paging::invalidate((void*)linear);
  return to_memflags(f);
};

Access protect(uintptr_t linear, Access flags)
{
  PG_PRINT("::protect 0x%lx \n", linear);
  x86::paging::Flags xflags = x86::paging::to_x86(flags);

  auto key = OS::memory_map().in_range(linear);

  // Throws if entry wasn't previously mapped.
  auto map_ent = OS::memory_map().at(key);

  PG_PRINT("Found entry: %s\n", map_ent.to_string().c_str());
  int sz_prot = 0;
  x86::paging::Flags fl;

  // TOOD: Optimize. No need to re-traverse for each page
  //       set_flags_r should probably just take size.
  while (sz_prot < map_ent.size()){
    auto page = linear + sz_prot;
    auto psize = active_page_size(page);
    PG_PRINT("Protecting page 0x%lx", page);
    fl |= __pml4->set_flags_r(page, xflags);
    sz_prot += psize;
    x86::paging::invalidate((void*)linear);
  }
  return to_memflags(fl);
};

Access flags(uintptr_t addr)
{
  return to_memflags(__pml4->flags_r(addr));
}

__attribute__((weak))
Map map(Map m, const char* name)
{
  using namespace x86::paging;
  using namespace util;

  if (UNLIKELY(!m))
    mem_fail_fast("Provided map was empty");

  if (UNLIKELY(m.lin == 0 or m.phys == 0))
    mem_fail_fast("Can't map the 0-page");

  if (UNLIKELY(not bits::is_aligned(min_psize(), m.lin)
               or not bits::is_aligned(min_psize(), m.phys)))
    mem_fail_fast("linear and physical must be aligned to supported page size");

  if (UNLIKELY(not bits::is_aligned(m.min_psize(), m.lin)
               or not bits::is_aligned(m.min_psize(), m.phys)))
    mem_fail_fast("linear and physical must be aligned to requested page size");

  if (UNLIKELY((m.page_sizes & supported_page_sizes()) == 0))
    mem_fail_fast("Requested page size(s) not supported");

  if (strcmp(name, "") == 0)
    name = "mem::map";

  OS::memory_map().assign_range({m.lin, m.lin + m.size - 1, name});

  auto new_map = __pml4->map_r(to_x86(m));
  if (new_map) {
    PG_PRINT("Wants : %s size: %li", m.to_string().c_str(), m.size);
    PG_PRINT("Gets  : %s size: %li", new_map.to_string().c_str(), new_map.size);

    // Size should match requested size rounded up to smallest requested page size
    Ensures(new_map.size == bits::roundto(m.min_psize(), m.size));
    Ensures(has_flag(to_memflags(new_map.flags), m.flags));
    Ensures(bits::is_aligned<4_KiB>(new_map.page_sizes));
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

    Ensures(m.size == util::bits::roundto<4_KiB>(map_ent.size()));
    OS::memory_map().erase(key);
  }

  return to_mmap(m);
}

uintptr_t active_page_size(uintptr_t addr){
  return __pml4->active_page_size(addr);
}

}} // os::mem

// must be public symbols because of a unittest...
extern char _TEXT_START_;
extern char _EXEC_END_;
uintptr_t __exec_begin = (uintptr_t)&_TEXT_START_;
uintptr_t __exec_end = (uintptr_t)&_EXEC_END_;

void allow_executable()
{
  INFO2("* Allowing execute on %p -> %p",
        (void*) __exec_begin, (void*)__exec_end);

  Expects(bits::is_aligned<4_KiB>(__exec_begin));
  Expects(__exec_end > __exec_begin);

  auto exec_size = __exec_end - __exec_begin;

  os::mem::Map m;
  m.lin        = __exec_begin;
  m.phys       = __exec_begin;
  m.size       = exec_size;
  m.page_sizes = os::mem::Map::any_size;
  m.flags      = os::mem::Access::execute | os::mem::Access::read;

  os::mem::map(m, "ELF .text");
}
