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
#include <lest.hpp>

extern "C" uintptr_t heap_begin;

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

            EXPECT(heap.in_use() < heap.size());

            // Resize heap to have only 1 MB of free space
            map.resize(heap_begin, heap.in_use() + 0x100000);

            EXPECT(heap.addr_end() < original_end);

            ptrdiff_t size = 1000;
            map.assign_range({heap.addr_end() + 1, heap.addr_end() + size,
                  "Custom", "A custom range next to the heap"});

            for (auto i: map)
              std::cout << i.second.to_string() << "\n";

            // TODO: This shouldn't work, but currently sbrk doesn't check the
            // actual memory map, only the heap_max it was initialized with.
            EXPECT_NOT(malloc(0x200000));

            auto* buf = malloc(0xf0000);
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
