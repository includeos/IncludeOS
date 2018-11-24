// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#include <common.cxx>
#include <iostream>
#include <kernel/memmap.hpp>
using namespace os::mem;

CASE ("Using the fixed memory range class")
{
    GIVEN ("A fixed memory range over raw sequence of bytes")
    {
      // A dynamically allocated range of bytes
      char* seq = const_cast<char*>("Spans provide a safer way to access memory");
      Fixed_memory_range range(reinterpret_cast<uintptr_t>(seq + 16),
                                 reinterpret_cast<uintptr_t>(seq + 20), "Demo range");

      auto seq_start = seq + 16;
      auto seq_end = seq + 20;
      EXPECT(range.size() == seq_end - seq_start + 1);
      EXPECT(std::string(range.name()) == std::string("Demo range"));

      THEN("You can iterate safely over that range, and not out of bounds"){
        for (auto i : range)
          EXPECT(i);

        auto i = range.begin();
        for (; i != range.end(); i++)
          EXPECT(*i);

        EXPECT_THROWS(++i);
        EXPECT_THROWS(i++);

      }

      AND_THEN("You can check if any address is within that range") {

        EXPECT(not range.in_range(1));
        EXPECT(not range.in_range((uintptr_t)seq));
        EXPECT(range.in_range((uintptr_t)seq_start));
        EXPECT(range.in_range((uintptr_t)seq_end));
        EXPECT(not range.in_range((uintptr_t)seq + 22));
      }

      AND_THEN("You can resize that range (but you should let the memory map do that)")
        {
          auto sz = range.size();
          range.resize(range.size() + 100);
          EXPECT(range.size() == sz + 100);
          range.resize(range.size() + 10000);
          EXPECT(range.size() == sz + 10100);
        }
    }
}

CASE("Construct a range with illegal parameters")
{
  EXPECT_THROWS((Fixed_memory_range{(uintptr_t)1000, (uintptr_t)10, "Negative - A range going backwards"}));
  EXPECT_THROWS((Fixed_memory_range{(uintptr_t)1, (uintptr_t)Fixed_memory_range::max_size() + 100, "Too big - A range > max of ptrdiff_t"}));
}

CASE ("Using the kernel memory map") {

  GIVEN ("An instance of the memory map with a named range")
    {
      Memory_map map{};
      EXPECT(map.size() == 0);
      EXPECT(not map.in_range(0));
      EXPECT(not map.in_range(0xff));
      EXPECT(not map.in_range(0xffff));
      uintptr_t init_key = 0xff;
      uintptr_t init_end = 0xffff;
      auto init_size = init_end - init_key + 1;
      map.assign_range({init_key, init_end, "Initial"});
      EXPECT(map.size() == 1);
      EXPECT(not map.empty());
      auto& range_init = map.at(init_key);
      EXPECT(range_init.bytes_in_use() == range_init.size());

      THEN("A copy of that range is identical")
        {
          auto& range = map.at(init_key);
          Fixed_memory_range my_copy = range;
          EXPECT(range.size() == my_copy.size());
          EXPECT(range.bytes_in_use() == my_copy.bytes_in_use());
          EXPECT(range.addr_start() == my_copy.addr_start());
          EXPECT(range.addr_end() == my_copy.addr_end());
          EXPECT(range.to_string() == my_copy.to_string());
        }

      AND_THEN("The range object can be moved")
        {
          auto range = map.at(init_key);
          const auto range_size         = range.size();
          const auto range_bytes_in_use = range.bytes_in_use();
          const auto range_addr_start   = range.addr_start();
          const auto range_addr_end     = range.addr_end();
          const auto range_string       = range.to_string();
          Fixed_memory_range thief      = std::move(range);
          EXPECT(range_size == thief.size());
          EXPECT(range_bytes_in_use == thief.bytes_in_use());
          EXPECT(range_addr_start == thief.addr_start());
          EXPECT(range_addr_end == thief.addr_end());
          EXPECT(range_string == thief.to_string());
        }

      AND_THEN("You can fetch that range using start address as key")
        {
          auto& range = map.at(init_key);
          EXPECT(range.size() == init_end - init_key + 1);

        }

      AND_THEN("You can ask the memory map if an address is in that range")
        {
          EXPECT(map.in_range(0xff1) == init_key);
          EXPECT(map.at(map.in_range(0xfff)).addr_start() == init_key);
          EXPECT(map.at(map.in_range(0xfffa)).addr_start() == init_key);
          EXPECT_NOT(map.in_range(2));
          EXPECT_NOT(map.in_range(0xf));
          EXPECT_NOT(map.in_range(0xffffff));

          // 0 is not a valid key (spans can start at address 0)
          EXPECT_THROWS(map.at(map.in_range(0)));
        }

      AND_THEN("You can increase the size of that range since nothing is above")
        {
          EXPECT(map.resize(init_key, init_size + 1) == init_size + 1);
          EXPECT(map.resize(init_key, init_size + 100) == init_size + 100);
        }

      AND_THEN("You can't downsize that range, since bytes_in_use is all of it by default")
        {
          EXPECT_THROWS(map.resize(init_key, init_size));
          EXPECT_THROWS(map.resize(init_key, 5));
        }

      AND_WHEN("You provide a custom bytes_in_use delegate you can downsize")
        {

          auto key = map.at(init_key).addr_end() + 1;
          map.assign_range({key, key + 1000, "Resizable", []()->ptrdiff_t{ return 4; }});


          auto res = map.resize(key, 5);
          EXPECT(res == 5);
          EXPECT(map.at(key).size() == res);
          EXPECT(map.resize(key, 100) == 100);
          EXPECT(map.resize(key, 200) == 200);
          EXPECT(map.resize(key, 20000) == 20000);

          THEN("You still can't downsize to less than what it currently uses")
            {

              EXPECT_THROWS(map.resize(key, 3));

              AND_THEN("If you assign a range above it, you can't increase it further")
                {

                  auto& last_range = map.at(key);
                  auto key2 = last_range.addr_end() + 1;
                  map.assign_range({key2, key2 + 1000, "Above Resizable"});

                  EXPECT_THROWS(map.resize(key, last_range.size() + 1));
                  EXPECT_THROWS(map.resize(key, last_range.size() + 100));
                }
            }
        }

      AND_THEN("You can't insert an overlapping range")
        {
          EXPECT_THROWS(map.assign_range({0x1, 0xfff, "Range 2 - Overlapping beginning of Range 1"}));
          EXPECT_THROWS(map.assign_range({0xfff, 0xfffff, "Range 2 - Overlapping end of Range 1"}));

          uintptr_t rng2_key = 0x30000;
          uintptr_t rng2_end = 0xfffff;
          EXPECT(map.assign_range({rng2_key, rng2_end, "Range 2 - OK range above initial"}).addr_start() == rng2_key);

          // A range where end-address only is with the second range
          EXPECT_THROWS(map.assign_range({0x20000, 0x40000, "Range 3 - Between Range 1 and Range 2"}));

          // A range where start-address is in the first of the two ranges
          EXPECT_THROWS(map.assign_range({0xfff0, 0x20000, "Range 3 - Between Range 1 and Range 2"}));

          /** Pretty print
          for (auto i : map)
            std::cout << "Range: " << i.second.to_string() << "\n";
          **/
        }

      AND_THEN("You can't insert an embedded  range")
        {
          EXPECT_THROWS(map.assign_range({0xfff, 0xfffa, "Range 4 - Embedded in range 1"}));
        }

      AND_WHEN("You've inserted several ranges you can iterate over them")
        {

          uintptr_t start = 0x10000;
          const size_t size = 100;
          const size_t count = 20;

          for (auto i = start; i < start + count * size; i += size) {
            Fixed_memory_range rng = {i , i + size - 1,"test"};
            map.assign_range(std::move(rng));
          }

          EXPECT(map.size() == count + 1);

          for (auto i : map) {
            if (std::string(i.second.name()) == "demo") {
              EXPECT (i.second.size() == 100);
            }
          }

          AND_THEN("Verify that the ranges are occupied")
            {
              uintptr_t addr = 0x10010;
              auto key = map.in_range(addr);

              EXPECT(key > 0);

              // Verify that every address in all the ranges report a conflict
              for (uintptr_t i = 0x10000; i < 0x10000 + count * size; i += 1){
                auto overlap = map.in_range(i);
                //std::cout << "i: " << std::hex << i << " overlaps with "  << overlap << "\n";
                EXPECT(overlap);
              }

              EXPECT(not map.in_range(0));
              EXPECT(not map.in_range(0x20000));
              EXPECT(not map.in_range(0x30000));
            }
        }
    }
}
