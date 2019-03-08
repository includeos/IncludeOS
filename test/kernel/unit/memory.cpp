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

// #define DEBUG_UNIT
#ifdef DEBUG_UNIT
#define MYINFO(X,...) INFO("<test os::mem>", X, ##__VA_ARGS__)
#else
#define MYINFO(X,...)
#endif

#include <common.cxx>
#include <iostream>
#include <kernel/memory.hpp>
#include <arch/x86/paging.hpp>
#include <malloc.h>
#include <os>

using namespace os;
using namespace util;

CASE ("Using os::mem::Mapping class")
{

  mem::Map m;

  // Empty map is convertible to false
  EXPECT(!m);

  // Map with size and page size is convertible to true
  m.size = 100;
  m.page_sizes = 4_KiB;
  EXPECT(m);

  m.page_sizes = 0;
  EXPECT(!m);

}


CASE("os::mem::Mapping Addition")
{
  mem::Map m{}, n{};
  const mem::Map zero{};
  EXPECT(!(m + n));
  EXPECT(m == n);
  EXPECT(m + n == m);

  m.size = 10;
  m.page_sizes = 4_KiB;

  auto k = m + n;
  EXPECT(k.size == 10);

  // Adding a zero-map has no effect
  EXPECT(k + zero == k);
  EXPECT(zero + k == k);

  // Adding non-contiguous linear address returns zero-map
  n.size = m.size;
  n.page_sizes = m.page_sizes;
  EXPECT(n + m == zero);

  // Adding maps with contiguous linear addressses result in added sizes
  n.lin = m.lin + m.size;
  auto old_m = m;
  auto old_n = n;
  m += n;
  EXPECT(old_m + old_n == m);
  EXPECT(m.size == old_m.size + old_n.size);
  EXPECT(m.lin == old_m.lin);

  n.size = 42;
  n.lin = m.lin + m.size;
  EXPECT((n + m).size == m.size + n.size);
  n += m;
  EXPECT(n.lin == m.lin);
  EXPECT((n.size == 42 + m.size and m.size == old_m.size + old_n.size));

}

CASE ("os::mem - Trying to map the 0 page")
{
  mem::Map m;
  m.lin = 0;
  m.phys = 4_GiB;
  m.size = 42_KiB;
  m.flags = mem::Access::read;

  // Throw due to assert fail in map
  EXPECT_THROWS(mem::map(m, "Fail"));

  m.lin = 4_GiB;
  m.phys = 0;

  // Throw due to assert fail in map
  EXPECT_THROWS(mem::map(m, "Fail"));
}

extern void  __arch_init_paging();
extern x86::paging::Pml4* __pml4;

// Default page setup RAII
class Default_paging {
public:
  ~Default_paging()
  {
    clear_paging();
  }

  Default_paging()
  {
    clear_paging();
    MYINFO("Initializing default paging \n");
    __arch_init_paging();
  }


  static void clear_paging() {
    using namespace x86::paging;
    MYINFO("Clearing default paging \n");
    if (__pml4 != nullptr) {
      __pml4->~Pml4();
      free(__pml4);
      __pml4 = nullptr;
      os::mem::vmmap().clear();
    }
  }
};


CASE ("os::mem Using map and unmap")
{
  using namespace util;

  Default_paging p{};
  auto initial_entries = os::mem::vmmap().map();

  // Create a desired mapping
  mem::Map m;
  m.lin = 5_GiB;
  m.phys = 4_GiB;
  m.size = 42_MiB;
  m.flags = mem::Access::read;
  m.page_sizes = 4_KiB | 2_MiB;

  // It shouldn't exist in the memory map
  auto key = os::mem::vmmap().in_range(m.lin);
  EXPECT(key == 0);

  // Map it and verify
  auto mapping = mem::map(m, "Unittest 1");

  EXPECT(mapping.lin == m.lin);
  EXPECT(mapping.phys == m.phys);
  EXPECT(mapping.flags == m.flags);
  EXPECT((mapping.page_sizes & m.page_sizes) != 0);
  EXPECT(os::mem::vmmap().map().size() == initial_entries.size() + 1);

  // Expect size is requested size rounded up to nearest page
  EXPECT(mapping.size == bits::roundto(4_KiB, m.size));

  // It should now exist in the OS memory map
  key = os::mem::vmmap().in_range(m.lin);
  EXPECT(key == m.lin);
  auto& entry = os::mem::vmmap().at(key);
  EXPECT(entry.size() == m.size);
  EXPECT(entry.name() == "Unittest 1");

  // You can't map in the same range twice
  EXPECT_THROWS(mem::map(m, "Unittest 2"));
  m.lin += 16_KiB;
  EXPECT_THROWS(mem::map(m, "Unittest 3"));

  // You can still map above
  m.lin += bits::roundto(4_KiB, m.size);
  EXPECT(mem::map(m, "Unittest 4").size == bits::roundto(4_KiB, m.size));
  EXPECT(mem::unmap(m.lin).size == bits::roundto(4_KiB, m.size));
  EXPECT(os::mem::vmmap().map().size() == initial_entries.size() + 1);

  // You can still map below
  m.lin = 5_GiB - bits::roundto(4_KiB, m.size);
  EXPECT(mem::map(m, "Unittest 5").size == bits::roundto(4_KiB, m.size));
  EXPECT(mem::unmap(m.lin).size == bits::roundto(4_KiB, m.size));
  EXPECT(os::mem::vmmap().map().size() == initial_entries.size() + 1);

  m.lin = 5_GiB;

  // Unmap and verify
  auto un = mem::unmap(m.lin);
  EXPECT(un.lin == mapping.lin);
  EXPECT(un.phys == 0);
  EXPECT(un.flags == mem::Access::none);
  EXPECT(un.size == mapping.size);
  key = os::mem::vmmap().in_range(m.lin);
  EXPECT(key == 0);

  // Remap and verify
  m.size = 42_MiB + 42_b;
  m.page_sizes = 2_MiB | 4_KiB;

  mapping = mem::map(m, "Unittest");
  EXPECT(mapping.lin == m.lin);
  EXPECT(mapping.phys == m.phys);
  EXPECT(mapping.flags == m.flags);
  EXPECT((mapping.page_sizes & m.page_sizes));
  EXPECT(mapping.size == bits::roundto<4_KiB>(m.size));
}


CASE ("os::mem using protect_range and flags")
{

  using namespace util;
  Default_paging p{};

  EXPECT(__pml4 != nullptr);
  mem::Map req = {6_GiB, 3_GiB, mem::Access::read, 15 * 4_KiB, 4_KiB};
  auto previous_flags = mem::flags(req.lin);
  mem::Map res = mem::map(req);
  EXPECT(req == res);
  EXPECT(mem::flags(req.lin) == mem::Access::read);
  EXPECT(mem::active_page_size(req.lin) == 4_KiB);

  auto page_below = req.lin - 4_KiB;
  auto page_above = req.lin + 4_KiB;
  mem::protect_page(page_below, mem::Access::none);
  EXPECT(mem::flags(page_below) == mem::Access::none);
  EXPECT(mem::active_page_size(page_below) >= 2_MiB);

  mem::protect_page(page_above, mem::Access::none);
  EXPECT(mem::flags(page_above) == mem::Access::none);
  EXPECT(mem::active_page_size(page_above) == 4_KiB);

  // The original page is untouched
  EXPECT(mem::flags(req.lin) == req.flags);

  // Can't protect a range that isn't mapped
  auto unmapped = 590_GiB;
  EXPECT(mem::flags(unmapped) == mem::Access::none);
  EXPECT_THROWS(mem::protect_range(unmapped, mem::Access::write | mem::Access::read));
  EXPECT(mem::flags(unmapped) == mem::Access::none);

  // You can still protect page
  EXPECT(mem::active_page_size(unmapped) == 512_GiB);
  auto rw = mem::Access::write | mem::Access::read;

  // But a 512 GiB page can't be present without being mapped
  EXPECT(mem::protect_page(unmapped, rw) == mem::Access::none);

  mem::protect_range(req.lin, mem::Access::execute);
  for (auto p = req.lin; p < req.lin + req.size; p += 4_KiB){
    EXPECT(mem::active_page_size(p) == 4_KiB);
    EXPECT(mem::flags(p) == (mem::Access::execute | mem::Access::read));
  }

  EXPECT(mem::flags(req.lin + req.size) == previous_flags);
}


CASE ("os::mem page table destructors")
{
  using namespace util;
  using namespace x86::paging;

  // Running this test with valgrind should show 0 bytes leaked.
  Default_paging p{};

  auto* ptr = new Pml4(0);
  EXPECT((uintptr_t(ptr) & ~(0x1000 - 1)) == uintptr_t(ptr));
  EXPECT(ptr->is_empty());
  EXPECT(ptr->bytes_allocated() == sizeof(Pml4));
  delete ptr;

  std::array<int*, 1000> ints {};
  std::array<Pml4*, 8> tbls {};

  Pml4* ars = new Pml4[8];
  delete[] ars;

  EXPECT(sizeof(Pml4) <= 4096 * 2);
  EXPECT(sizeof(ars)  <= 8 * sizeof(Pml4));
  std::array<Pml4, 8> stack_tbls{{0}};

  int it = 0;
  for (auto& tbl : tbls)
  {
    tbl = new Pml4(0);
    EXPECT(bits::is_aligned<Pml4::Arch::min_pagesize>(tbl));
  }

  for (auto& tbl : tbls) delete tbl;


}


CASE ("os::mem using protect")
{
SETUP ("Assuming a default page table setup")
{
  using namespace util;
  Default_paging p{};

  EXPECT(__pml4 != nullptr);

  SECTION ("os::mem protect basics")
  {
    // Save some page sizes around the area we'll be mapping
    std::array<uintptr_t, 4> sizes_pre {mem::active_page_size(5_GiB),
                            mem::active_page_size(6_GiB),
                            mem::active_page_size(6_GiB + 100_MiB),
                            mem::active_page_size(7_GiB)};

    // You can't protect an unmapped range
    EXPECT_THROWS(mem::protect(6_GiB + 900_MiB, 300_MiB, mem::Access::read));

    // Map something (a lot will be mapped by default in IncludeOS)
    mem::Map req {6_GiB, 3_GiB, mem::Access::read | mem::Access::write, 300_MiB};
    auto res = mem::map(req);
    EXPECT(res);
    EXPECT(res.flags == (mem::Access::write | mem::Access::read));
    EXPECT(res.lin == req.lin);
    EXPECT(res.phys == req.phys);

    // You can't protect a partially mapped range
    EXPECT_THROWS(mem::protect(5_GiB + 900_MiB, 300_MiB, mem::Access::read | mem::Access::write));
    auto prot_offs  = 100_MiB;
    auto prot_begin = req.lin + prot_offs;
    auto prot_size  = 12_KiB;

    EXPECT(mem::virt_to_phys(req.lin)  == req.phys);
    EXPECT(mem::virt_to_phys(prot_begin) == req.phys + prot_offs);

    // You can protect a subset of a mapped range
    auto pres = mem::protect(prot_begin, prot_size);

    EXPECT(pres);
    EXPECT(pres.flags       == mem::Access::read);
    EXPECT(pres.lin         == prot_begin);
    EXPECT(pres.size        == prot_size);
    EXPECT(pres.page_sizes  == 4_KiB);
    EXPECT(pres.phys        == res.phys + prot_offs);

    uintptr_t p = pres.phys;
    // The new range will have the smallest page size
    for (uintptr_t v = prot_begin; v < prot_begin + prot_size; v += 4_KiB, p += 4_KiB)
    {
      EXPECT(mem::active_page_size(v) == 4_KiB);
      EXPECT(mem::virt_to_phys(v) == p);
    }

    // The rest of memory is largely as before except adjacent areas
    std::array<uintptr_t, 4> sizes_post {mem::active_page_size(5_GiB),
                             mem::active_page_size(6_GiB),
                             mem::active_page_size(prot_begin),
                             mem::active_page_size(7_GiB)};

    EXPECT(sizes_pre[0] == sizes_post[0]); // Can't have changed, 1 GiB below
    EXPECT(sizes_pre[1] >= sizes_post[1]); // Is now likely 2_MiB or 4_KiB
    EXPECT(sizes_pre[2] >  sizes_post[2]); // Must be the new size
    EXPECT(sizes_pre[3] == sizes_post[3]); // Can't have changed, 1 GiB above
  }
}
}

CASE("os::mem::protect try to break stuff"){
  using namespace util::literals;
  auto init_access = mem::Access::none;
  Default_paging::clear_paging();

  EXPECT(__pml4 == nullptr);
  __pml4 = new x86::paging::Pml4(0);
  EXPECT(__pml4->is_empty());
  auto initial_use = __pml4->bytes_allocated();
  MYINFO("Initial memory use: %zi \n", initial_use);

  for (auto r : test::random) {
    auto lin  = 3 % 2 ? (uintptr_t)r % (1_GiB) : (uintptr_t)r % 2_MiB;
    auto phys = 1_MiB + r % 100_MiB;
    auto size = 4_KiB + (r % 2 ? r % 2_GiB : r % 4_MiB);

    mem::Map req;
    req.lin   = util::bits::roundto<4_KiB>(lin);
    req.phys  = util::bits::roundto<4_KiB>(phys);
    req.flags = mem::Access::none;
    req.size  = util::bits::roundto<4_KiB>(size);
    req.page_sizes = mem::Map::any_size;

    if (r % 3 == 0)
      req.flags |= mem::Access::read;
    if (r % 3 == 1)
      req.flags |= mem::Access::write;
    if (r % 3 == 2)
      req.flags |= mem::Access::execute;

    auto m = mem::map(req);
    EXPECT(m);
    auto bytes_after_map = __pml4->bytes_allocated();
    EXPECT(bytes_after_map > initial_use);

    MYINFO("Allocated bytes after map: %zi == %zi tables\n",
           bytes_after_map, bytes_after_map / sizeof(decltype(*__pml4)));

    // Unmap
    mem::unmap(m.lin);
    EXPECT(__pml4->bytes_allocated() <= bytes_after_map);
    auto bytes_after_unmap = __pml4->bytes_allocated();
    MYINFO("Allocated bytes after unmap: %zi == %zi tables\n",
           bytes_after_unmap, bytes_after_unmap / sizeof(decltype(*__pml4)));

    // Purge unused
    __pml4->purge_unused();

    auto bytes_after_purge = __pml4->bytes_allocated();
    MYINFO("Allocated bytes after purge: %zi == %zi tables\n",
           bytes_after_purge, bytes_after_purge / sizeof(decltype(*__pml4)));

    EXPECT(__pml4->bytes_allocated() == initial_use);
  }
  MYINFO("Allocated bytes at end: %zi \n", __pml4->bytes_allocated());
  Default_paging::clear_paging();
}


CASE("os::mem::protect verify consistency"){
  using namespace util::literals;
  auto init_access = mem::Access::none;

  if (__pml4 != nullptr) {
    printf("NOT NULL\n");
  }
  EXPECT(__pml4 == nullptr);

  __pml4 = new x86::paging::Pml4(0);
  EXPECT(__pml4->is_empty());

  auto initial_use = __pml4->bytes_allocated();
  MYINFO("Initial memory use: %zi \n", initial_use);


  mem::Map req {6_GiB, 3_GiB, mem::Access::read | mem::Access::write, 300_MiB};
  auto res = mem::map(req);
  EXPECT(res);
  EXPECT(res.flags == (mem::Access::write | mem::Access::read));
  EXPECT(res.lin == req.lin);
  EXPECT(res.phys == req.phys);

  auto prot_offs  = 1_MiB;
  auto prot_begin = 6_GiB + prot_offs;
  auto prot_size  = 1043_KiB;
  auto diff_phys  = req.lin - req.phys;
  auto new_flags  = mem::Access::read;

  // Write-protect
  auto prot = mem::protect(prot_begin, prot_size, new_flags);
  EXPECT(prot);
  EXPECT(prot.flags == new_flags);

  // Verify each page
  for (auto i = prot_begin; i < prot_begin + prot_size; i += mem::min_psize()) {
    EXPECT(mem::virt_to_phys(i) == i - diff_phys);
    auto flags = mem::flags(i);
    EXPECT(flags == new_flags);
  }

  auto memuse_after_prot = __pml4->bytes_allocated();
  EXPECT(__pml4->bytes_allocated() > initial_use);

  // Protect with different flags
  new_flags = mem::Access::read | mem::Access::write | mem::Access::execute;
  auto prot2 = mem::protect(prot_begin, prot_size, new_flags);
  EXPECT(prot2);

  // Verify each page
  for (auto i = prot_begin; i < prot_begin + prot_size; i += mem::min_psize()) {
    EXPECT(mem::virt_to_phys(i) == i - diff_phys);
    auto flags = mem::flags(i);
    EXPECT(flags == new_flags);
  }

  EXPECT(__pml4->bytes_allocated() == memuse_after_prot);

  Default_paging::clear_paging();
}
