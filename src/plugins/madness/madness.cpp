// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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
#include <timers>

#include "madness.hpp"
#include "doge.hpp"

#define RANT(TEST, TEXT, ...) \
  printf("%16s[%s] " TEXT "\n","", TEST ? "+" : " ",  ##__VA_ARGS__); \

namespace madness {

  static std::vector<const char*> allocations{};
  static int32_t alloc_timer   = 0;
  static int32_t dealloc_timer = 0;
  static int64_t bytes_held    = 0;

  bool do_allocs = true;

  void init() {
    INFO("Madness", "Initializing...\n%s\n", doge_txt);
    init_status_printing();
    init_heap_steal();
  }

  void init_heap_steal() {
    INFO("Madness", "Starting allocation timer every %isec.", alloc_freq);
    Timers::periodic(alloc_freq, alloc_freq, [](uint32_t id) {
        if (alloc_timer == 0)
          alloc_timer = id;

        if (not do_allocs)
          return;

        if (OS::heap_avail() < alloc_min)
          return;

        auto  sz  = std::max(OS::heap_avail() / 4, alloc_min);
        auto* ptr = malloc(sz);

        if (ptr == nullptr and sz > alloc_min) {
          sz = alloc_min;
          ptr = malloc(sz);
        }

        if (ptr == nullptr) {
          INFO("Madness", "Allocation of %s failed. Available heap: %s",
               util::Byte_r(sz).to_string().c_str(),
               util::Byte_r(OS::heap_avail()).to_string().c_str());
          INFO("Madness", "Cleaning up in %isec \n", dealloc_delay);
          auto* first = allocations.front();
          allocations.erase(allocations.begin());
          free((void*)first);
          Timers::oneshot(dealloc_delay, [](uint32_t id) {
              INFO("Madness", "Cleaning up %i allocations\n", dealloc_delay);
              for (auto* a : allocations)
                free((void*)a);
              allocations.clear();
              bytes_held = 0;
            });

          do_allocs = false;
          Timers::oneshot(alloc_restart_delay, [](auto id) {
              do_allocs = true;
            });

          return;
        }

        bytes_held += sz;
        INFO("Madness", "Allocated %s @ %p. Available heap: %s",
             util::Byte_r(sz).to_string().c_str(), ptr,
             util::Byte_r(OS::heap_avail()).to_string().c_str());

        allocations.push_back((char*)ptr);

      });

  }

  void init_status_printing() {
    Timers::periodic(1s, alloc_freq, [](auto id) {
        static int i = 0;
        INFO("Madness", "Runtime: %is. Available heap: %s. Occupied by me: %s",
             i++ * alloc_freq.count(),
             util::Byte_r(OS::heap_avail()).to_string().c_str(),
             util::Byte_r(bytes_held).to_string().c_str());
      });
  }
}

__attribute__ ((constructor))
void madness_plugin() {
  OS::register_plugin(madness::init, "Madness");
}
