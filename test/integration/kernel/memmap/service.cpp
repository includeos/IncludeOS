// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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
#include <kernel/memory.hpp>
#include <util/bitops.hpp>


using namespace util;

void Service::start(const std::string&)
{
  INFO("Memmap", "Testing the kernel memory map");

  auto& map = os::mem::vmmap();
  Expects(map.size());

  // Verify that you can't create any overlapping ranges
  const auto s = map.size();
  size_t failed = 0;
  auto i = 0;
  for (auto it : map) {
    try {
      auto m = it.second;
      Expects(m.size());
      int offs   = rand() % m.size() + 1;
      uintptr_t begin = i++ % 2 ? m.addr_start() + offs : m.addr_start() - offs;
      uintptr_t end   = begin + offs * 2;
      os::mem::Fixed_memory_range rng {begin, end, "Can't work"};
      map.assign_range(std::move(rng));
    } catch (const std::exception& e) {
      failed++;
    }
  }

  Expects(failed == s);
  Expects(map.size() == s);

  // mem::map is using memory_map to keep track of virutal memory
  // TODO: we might consider consolidating ranges with mappings.
  auto m = os::mem::map({42_GiB, 42_GiB, os::mem::Access::read, 1_GiB},
                        "Test range");
  Expects(m);
  Expects(map.size() == s + 1);

  auto key = map.in_range(42_GiB);
  Expects(key == 42_GiB);
  auto rng = map.at(key);
  Expects(rng.size() == 1_GiB);

  // TODO: There's an unimplemented map.assign_range(size) to integrate.
  // auto& given = map.assign_range(5_GiB);
  // Expects(given.size() == 5_GiB);

  INFO("Memmap", "SUCCESS");

}
