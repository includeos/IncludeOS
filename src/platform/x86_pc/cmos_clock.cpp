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

#include "cmos_clock.hpp"
#include "cmos.hpp"
#include <kernel/timers.hpp>
#include <kernel/os.hpp>
#include <hertz>

namespace x86
{
  static int64_t  current_time;
  static uint64_t current_ticks;

  void CMOS_clock::init()
  {
    using namespace std::chrono;
    current_time  = CMOS::now().to_epoch();
    current_ticks = OS::cycles_since_boot();

    INFO("CMOS", "Enabling regular clock sync for CMOS clock");
    // every minute recalibrate
    Timers::periodic(seconds(60), seconds(60),
      [] (Timers::id_t) {
        current_time  = CMOS::now().to_epoch();
        current_ticks = OS::cycles_since_boot();
      });
  }

  int64_t CMOS_clock::system_time()
  {
    auto ticks = OS::cycles_since_boot() - current_ticks;
    auto diff  = ticks / Hz(MHz(OS::cpu_freq())).count();

    return current_time + diff;
  }
}
