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
#include <lest/lest.hpp>
#include <util/bitops.hpp>

extern "C" uintptr_t heap_begin;
extern "C" uintptr_t heap_end;

using namespace util;

const lest::test specification[] =
  {
    {
      CASE( "Make sure the heap respects the memory map max" )
      {
        auto heap = OS::memory_map().at(heap_begin);
        EXPECT(heap.addr_start());
        EXPECT(heap.addr_end() == OS::heap_max());
        auto* buf = malloc(0x100000);
        EXPECT(buf);
        free(buf);
        EXPECT_NOT(malloc(heap.addr_end()));
        EXPECT(errno == ENOMEM);
      }
    },
    {
      SCENARIO ("Using the kernel memory map")
      {
        GIVEN ("The kernel map")
        {
          auto& map = OS::memory_map();
          EXPECT(map.size());

          THEN("You can resize the heap as long as it's not full")
          {
            auto& heap = map.at(heap_begin);
            auto original_end = heap.addr_end();
            EXPECT(original_end == OS::heap_max());
            auto in_use_prior = bits::roundto(4_KiB, heap.bytes_in_use());

            EXPECT(heap.bytes_in_use() < heap.size());

            // Resize heap to have only 1 MB of free space
            auto new_size = in_use_prior + 1_MiB;
            OS::resize_heap(new_size);

            EXPECT(heap.addr_end() == heap.addr_start() + in_use_prior + 1_MiB - 1);
            EXPECT(heap.addr_end() == OS::heap_max() - 1);

            ptrdiff_t size = 1000;
            map.assign_range({OS::heap_max(), OS::heap_max() + size - 1, "Custom"});

            for (auto i: map)
              std::cout << i.second.to_string() << "\n";

            auto capacity = heap.addr_end() - heap.addr_start() + heap.bytes_in_use();
            EXPECT(capacity > 1_MiB);

            EXPECT_NOT(malloc(2_MiB));

            // Malloc might ask sbrk for more than it needs
            auto* buf = malloc(500_KiB);
            EXPECT(buf);
          }
        }
      }
    }
  };


void Service::start(const std::string&)
{
  INFO("Memmap", "Testing the kernel memory map");

  auto failed = lest::run(specification, {"-p"});
  Expects(not failed);

  INFO("Memmap", "SUCCESS");

}
