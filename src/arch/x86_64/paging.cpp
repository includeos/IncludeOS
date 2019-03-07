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
#include <arch/x86/paging.hpp>
#include <arch/x86/paging_utils.hpp>
#include <kernel/cpuid.hpp>
#include <kprint>

// #define DEBUG_X86_PAGING
#ifdef DEBUG_X86_PAGING
#define MEM_PRINT(X, ...) kprintf("<Paging> " X, __VA_ARGS__)
#else
#define MEM_PRINT(X, ...)
#endif

template<>
const size_t  x86::paging::Map::any_size { supported_page_sizes() };

template<>
const size_t  os::mem::Map::any_size { supported_page_sizes() };

using namespace os::mem;
using namespace util;

using Flags = x86::paging::Flags;
using Pml4  = x86::paging::Pml4;

static void allow_executable();
static void protect_pagetables_once();

// must be public symbols because of a unittest
#ifndef PLATFORM_UNITTEST
extern char _TEXT_START_;
extern char _EXEC_END_;
uintptr_t __exec_begin = (uintptr_t)&_TEXT_START_;
uintptr_t __exec_end = (uintptr_t)&_EXEC_END_;
#else
extern uintptr_t __exec_begin;
extern uintptr_t __exec_end;
#endif

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
  auto default_fl = Flags::present | Flags::writable | Flags::huge | Flags::no_exec;
  __pml4 = new Pml4(0);
  Expects(__pml4 != nullptr);
  Expects(!__pml4->has_flag(0, Flags::present));

  INFO2("* Supported page sizes: %s", os::mem::page_sizes_str(os::mem::Map::any_size).c_str());

  INFO2("* Adding 512 1GiB entries @ 0x0 -> 0x%llx", 512_GiB);
  auto* pml3_0 =  __pml4->create_page_dir(0, 0, default_fl);

  Expects(pml3_0 != nullptr);
  Expects(pml3_0->has_flag(0,Flags::present));

  Expects(__pml4->has_flag(0, Flags::present));
  /*FIXME TODO this test is not sane when we do not control the heap in unittests
  Expects(__pml4->has_flag((uintptr_t)pml3_0, Flags::present));
  */

  if (not os::mem::supported_page_size(1_GiB)) {
    auto first_range = __pml4->map_r({0,0,default_fl, 16_GiB});
    Expects(first_range && first_range.size >= 16_GiB);
    INFO2("* Identity mapping %s", first_range.to_string().c_str());
  }

  INFO2("* Marking page 0 as not present");
  auto zero_page = __pml4->map_r({0, 0, Flags::none, 4_KiB, 4_KiB});
  Expects(__pml4->has_flag(8_KiB, Flags::present | Flags::writable | Flags::no_exec));
  allow_executable();

  Expects(zero_page.size == 4_KiB);
  Expects(zero_page.page_sizes == 4_KiB);
  Expects(__pml4->active_page_size(0LU) == 4_KiB);
  Expects(! __pml4->has_flag(0, Flags::present));

  Expects(! __pml4->has_flag((uintptr_t)__exec_begin, Flags::no_exec));
  Expects(__pml4->has_flag((uintptr_t)__exec_begin, Flags::present));

  // hack to see who overwrites the pagetables
  //protect_pagetables_once();

  INFO2("* Passing page tables to CPU");
  extern void __x86_init_paging(void*);
  __x86_init_paging(__pml4->data());
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
  } else {
    return Flags::none;
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

} // mem
} // os

using namespace os;

uintptr_t mem::supported_page_sizes() {
  static size_t res = 0;
  if (res == 0) {
    res = 4_KiB | 2_MiB;
    if (CPUID::has_feature(CPUID::Feature::PDPE1GB))
      res |= 1_GiB;
  }
  return  res;
}

uintptr_t mem::min_psize()
{
  return bits::keepfirst(supported_page_sizes());
}

uintptr_t mem::max_psize()
{
  return bits::keeplast(supported_page_sizes());
}

bool mem::supported_page_size(uintptr_t size)
{
  return bits::is_pow2(size) and (size & supported_page_sizes()) != 0;
}

Map to_mmap(Map_x86 map){
  return {map.lin, map.phys, to_memflags(map.flags), map.size, map.page_sizes};
}

Map_x86 to_x86(Map map){
  return {map.lin, map.phys, x86::paging::to_x86(map.flags), map.size, map.page_sizes};
}

uintptr_t mem::virt_to_phys(uintptr_t linear)
{
  auto* ent = __pml4->entry_r(linear);
  if (ent == nullptr) return 0;
  return __pml4->addr_of(*ent);
}

Access mem::protect_page(uintptr_t linear, Access flags)
{
  MEM_PRINT("::protect_page 0x%lx\n", linear);
  x86::paging::Flags xflags = x86::paging::to_x86(flags);
  auto f = __pml4->set_flags_r(linear, xflags);
  x86::paging::invalidate((void*)linear);
  return to_memflags(f);
};

Access mem::protect_range(uintptr_t linear, Access flags)
{
  MEM_PRINT("::protect 0x%lx \n", linear);
  x86::paging::Flags xflags = x86::paging::to_x86(flags);

  auto key = os::mem::vmmap().in_range(linear);

  // Throws if entry wasn't previously mapped.
  auto map_ent = os::mem::vmmap().at(key);

  MEM_PRINT("Found entry: %s\n", map_ent.to_string().c_str());
  int sz_prot = 0;
  x86::paging::Flags fl{};

  // TOOD: Optimize. No need to re-traverse for each page
  //       set_flags_r should probably just take size.
  while (sz_prot < map_ent.size()){
    auto page = linear + sz_prot;
    auto psize = active_page_size(page);
    MEM_PRINT("Protecting page 0x%lx", page);
    fl |= __pml4->set_flags_r(page, xflags);
    sz_prot += psize;
    x86::paging::invalidate((void*)linear);
  }
  return to_memflags(fl);
};

Map mem::protect(uintptr_t linear, size_t len, Access flags)
{
  if (UNLIKELY(len < min_psize()))
    mem_fail_fast("Can't map less than a page\n");

  if (UNLIKELY(linear == 0))
    mem_fail_fast("Can't map to address 0");

  MEM_PRINT("::protect 0x%lx \n", linear);
  auto key = os::mem::vmmap().in_range(linear);
  MEM_PRINT("Found key: 0x%zx\n", key);
  // Throws if entry wasn't previously mapped.
  auto map_ent = os::mem::vmmap().at(key);
  MEM_PRINT("Found entry: %s\n", map_ent.to_string().c_str());

  auto xflags = x86::paging::to_x86(flags);
  x86::paging::Map m{linear, x86::paging::any_addr, xflags, static_cast<size_t>(len)};
  MEM_PRINT("Wants: %s \n", m.to_string().c_str());

  Expects(m);
  auto res = __pml4->map_r(m);
  return to_mmap(res);
}

Access mem::flags(uintptr_t addr)
{
  return to_memflags(__pml4->flags_r(addr));
}

__attribute__((weak))
Map mem::map(Map m, const char* name)
{
  using namespace x86::paging;
  using namespace util;
  MEM_PRINT("Wants : %s size: %li \n", m.to_string().c_str(), m.size);
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

  // Align size to minimal page size;
  auto req_addr_end = m.lin + bits::roundto(m.min_psize(), m.size) - 1;

  os::mem::vmmap().assign_range({m.lin, req_addr_end, name});

  auto new_map = __pml4->map_r(to_x86(m));
  if (new_map) {
    MEM_PRINT("Gets  : %s size: %li\n", new_map.to_string().c_str(), new_map.size);

    // Size should match requested size rounded up to smallest requested page size
    Ensures(new_map.size == bits::roundto(m.min_psize(), m.size));
    Ensures(has_flag(to_memflags(new_map.flags), m.flags));
    Ensures(bits::is_aligned<4_KiB>(new_map.page_sizes));
  }

  return to_mmap(new_map);
};

Map mem::unmap(uintptr_t lin){
  auto key = os::mem::vmmap().in_range(lin);
  Map_x86 m;
  if (key) {
    MEM_PRINT("mem::unmap %p \n", (void*)lin);
    auto map_ent = os::mem::vmmap().at(key);
    m.lin = lin;
    m.phys = 0;
    m.size = map_ent.size();

    m = __pml4->map_r({key, 0, x86::paging::to_x86(Access::none), (size_t)map_ent.size()});

    Ensures(m.size == util::bits::roundto<4_KiB>(map_ent.size()));
    os::mem::vmmap().erase(key);
  }

  return to_mmap(m);
}

uintptr_t mem::active_page_size(uintptr_t addr){
  return __pml4->active_page_size(addr);
}



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

void protect_pagetables_once()
{
  struct ptinfo {
    void*  addr;
    size_t len;
  };
  std::vector<ptinfo> table_entries;

  __pml4->traverse(
    [&table_entries] (void* ptr, size_t len) {
      table_entries.push_back({ptr, len});
    });


  for (auto it = table_entries.rbegin(); it != table_entries.rend(); ++it)
  {
    const auto& entry = *it;
    const x86::paging::Map m {
        (uintptr_t) entry.addr, x86::paging::any_addr,
        x86::paging::Flags::present, static_cast<size_t>(entry.len)
      };
    assert(m);
    MEM_PRINT("Protecting table: %p, size %zu\n", entry.addr, entry.len);
    __pml4->map_r(m);
  }
}
