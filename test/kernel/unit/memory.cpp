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

#include <common.cxx>
#include <iostream>
#include <kernel/memory.hpp>
#include <os>

using namespace os;
using namespace util;

CASE ("Using os::mem::Mapping class")
{

  mem::Mapping m;

  // Empty map is convertible to false
  EXPECT(!m);

  // Map with size and page size is convertible to true
  m.size = 100;
  m.page_size = 4_KiB;
  EXPECT(m);

  // Correct page count is calculated
  EXPECT(m.page_count() == 1);
  m.size = 4097;
  EXPECT(m.page_count() == 2);

  for (int i = 0; i < 10; i++) {
    m.size = rand();
    auto cnt = (m.size / m.page_size)
      + (m.size % m.page_size ? 1 : 0);
    EXPECT(m.page_count() == cnt);
  }
}


CASE("os::mem::Mapping Addition")
{

  mem::Mapping m{}, n{};
  const mem::Mapping zero{};
  EXPECT(!(m + n));
  EXPECT(m == n);
  EXPECT(m + n == m);

  m.size = 10;
  m.page_size = 4_KiB;

  auto k = m + n;
  EXPECT(k.size == 10);

  // Adding a zero-map has no effect
  EXPECT(k + zero == k);
  EXPECT(zero + k == k);

  // Adding non-contiguous linear address returns zero-map
  n.size = m.size;
  n.page_size = m.page_size;
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

CASE ("Trying to map the 0 page")
{
  mem::Mapping m;
  m.lin = 0;
  m.phys = 4_GiB;
  m.size = 42_KiB;
  m.flags = mem::Access::Read;

  // Throw due to assert fail in map
  EXPECT_THROWS(mem::map(m, "Fail"));

  m.lin = 4_GiB;
  m.phys = 0;

  // Throw due to assert fail in map
  EXPECT_THROWS(mem::map(m, "Fail"));
}

CASE ("Using mem::map and mem::unmap")
{
  extern void  __arch_init_paging();
  extern void* __pml4;
  if (__pml4 == nullptr)
    __arch_init_paging();

  auto initial_entries = OS::memory_map().map();

  // Create a desired mapping
  mem::Mapping m;
  m.lin = 5_GiB;
  m.phys = 4_GiB;
  m.size = 42_KiB;
  m.flags = mem::Access::Read;
  m.page_size = 4_KiB;

  // It shouldn't exist in the memory map
  auto key = OS::memory_map().in_range(m.lin);
  EXPECT(key == 0);

  std::cout << "Using mem::map: Mapping " << Byte_r(m.size) << "\n";
  // Map it and verify
  auto mapping = mem::map(m, "Unittest");
  EXPECT(mapping.lin == m.lin);
  EXPECT(mapping.phys == m.phys);
  EXPECT(mapping.flags == m.flags);
  EXPECT(mapping.page_size == m.page_size);
  EXPECT(OS::memory_map().map().size() == initial_entries.size() + 1);

  // Expect size is requested size rounded up to nearest page
  EXPECT(mapping.size == roundto(m.page_size, m.size));

  // It should now exist in the OS memory map
  key = OS::memory_map().in_range(m.lin);
  EXPECT(key == m.lin);
  auto& entry = OS::memory_map().at(key);
  EXPECT(entry.size() == m.size);
  EXPECT(entry.name() == "Unittest");

  // You can't map in the same range twice
  EXPECT_THROWS(mem::map(m, "Unittest"));
  m.lin += 16_KiB;
  EXPECT_THROWS(mem::map(m, "Unittest"));

  // You can still map above
  m.lin += 28_KiB;
  EXPECT(mem::map(m, "Unittest").size == roundto(m.page_size, m.size));
  EXPECT(mem::unmap(m.lin).size == roundto(m.page_size, m.size));
  EXPECT(OS::memory_map().map().size() == initial_entries.size() + 1);

  // You can still map below
  m.lin -= (44_KiB + roundto<4_KiB>(m.size));
  EXPECT(mem::map(m, "Unittest").size == roundto(m.page_size, m.size));
  EXPECT(mem::unmap(m.lin).size == roundto(m.page_size, m.size));
  EXPECT(OS::memory_map().map().size() == initial_entries.size() + 1);

  m.lin = 5_GiB;

  // Unmap and verify
  auto un = mem::unmap(m.lin);
  EXPECT(un.lin == mapping.lin);
  EXPECT(un.phys == 0);
  EXPECT(un.flags == mem::Access::None);
  EXPECT(un.size == mapping.size);
  key = OS::memory_map().in_range(m.lin);
  EXPECT(key == 0);

  // Remap and verify
  m.size = 42_MiB + 42_b;
  m.page_size = 2_MiB;

  mapping = mem::map(m, "Unittest");
  EXPECT(mapping.lin == m.lin);
  EXPECT(mapping.phys == m.phys);
  EXPECT(mapping.flags == m.flags);
  EXPECT((mapping.page_size & m.page_size));
  EXPECT(mapping.size == roundto<4_KiB>(m.size));
}
