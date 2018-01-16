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

#define DEBUG_UNIT

#include <common.cxx>
#include <iostream>
#include <kernel/memory.hpp>
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

  // Correct page count is calculated
  EXPECT(m.page_count() == 1);
  m.size = 4097;
  EXPECT(m.page_count() == 2);

  for (int i = 0; i < 10; i++) {
    m.size = rand();
    auto cnt = (m.size / m.page_sizes)
      + (m.size % m.page_sizes ? 1 : 0);
    EXPECT(m.page_count() == cnt);
  }
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

  EXPECT(m + n == (m += n));
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
extern void* __pml4;

static void init_default_paging()
{
  if (__pml4 == nullptr) {
    free(__pml4);
    __arch_init_paging();
  }
}


CASE ("os::mem Using map and unmap")
{
  using namespace util;

  init_default_paging();
  auto initial_entries = OS::memory_map().map();

  // Create a desired mapping
  mem::Map m;
  m.lin = 5_GiB;
  m.phys = 4_GiB;
  m.size = 42_MiB;
  m.flags = mem::Access::read;
  m.page_sizes = 4_KiB | 2_MiB;

  // It shouldn't exist in the memory map
  auto key = OS::memory_map().in_range(m.lin);
  EXPECT(key == 0);

  // Map it and verify
  auto mapping = mem::map(m, "Unittest 1");

  EXPECT(mapping.lin == m.lin);
  EXPECT(mapping.phys == m.phys);
  EXPECT(mapping.flags == m.flags);
  EXPECT((mapping.page_sizes & m.page_sizes) != 0);
  EXPECT(OS::memory_map().map().size() == initial_entries.size() + 1);

  // Expect size is requested size rounded up to nearest page
  EXPECT(mapping.size == bits::roundto(4_KiB, m.size));

  // It should now exist in the OS memory map
  key = OS::memory_map().in_range(m.lin);
  EXPECT(key == m.lin);
  auto& entry = OS::memory_map().at(key);
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
  EXPECT(OS::memory_map().map().size() == initial_entries.size() + 1);

  // You can still map below
  m.lin = 5_GiB - bits::roundto(4_KiB, m.size);
  EXPECT(mem::map(m, "Unittest 5").size == bits::roundto(4_KiB, m.size));
  EXPECT(mem::unmap(m.lin).size == bits::roundto(4_KiB, m.size));
  EXPECT(OS::memory_map().map().size() == initial_entries.size() + 1);

  m.lin = 5_GiB;

  // Unmap and verify
  auto un = mem::unmap(m.lin);
  EXPECT(un.lin == mapping.lin);
  EXPECT(un.phys == 0);
  EXPECT(un.flags == mem::Access::none);
  EXPECT(un.size == mapping.size);
  key = OS::memory_map().in_range(m.lin);
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


CASE ("os::mem using protect and flags")
{

  using namespace util;

  init_default_paging();
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
  EXPECT(mem::active_page_size(page_below) == 2_MiB);

  mem::protect_page(page_above, mem::Access::none);
  EXPECT(mem::flags(page_above) == mem::Access::none);
  EXPECT(mem::active_page_size(page_above) == 4_KiB);

  // The original page is untouched
  EXPECT(mem::flags(req.lin) == req.flags);

  // Can't protect a range that isn't mapped
  auto unmapped = 590_GiB;
  EXPECT(mem::flags(unmapped) == mem::Access::none);
  EXPECT_THROWS(mem::protect(unmapped, mem::Access::write | mem::Access::read));
  EXPECT(mem::flags(unmapped) == mem::Access::none);

  // You can still protect page
  EXPECT(mem::active_page_size(unmapped) == 512_GiB);
  auto rw = mem::Access::write | mem::Access::read;

  // But a 512 GiB page can't be present without being mapped
  EXPECT(mem::protect_page(unmapped, rw) == mem::Access::none);

  mem::protect(req.lin, mem::Access::execute);
  for (auto p = req.lin; p < req.lin + req.size; p += 4_KiB){
    EXPECT(mem::active_page_size(p) == 4_KiB);
    EXPECT(mem::flags(p) == (mem::Access::execute | mem::Access::read));
  }

  EXPECT(mem::flags(req.lin + req.size) == previous_flags);
}
