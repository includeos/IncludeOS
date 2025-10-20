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
#include <cassert>
#include <smp>
#include <timers>
#include <kernel/events.hpp>
#include <string.h>
#include <malloc.h>
#include <expects>

static minimal_barrier_t barry;

void Service::start()
{

  barry.reset(0);

  printf("Testing memory allocation...\n");

  for (const auto& i : SMP::active_cpus())
  {
    SMP::global_lock();
    printf("CPU %i active \n", i);
    SMP::global_unlock();

    SMP::add_task([i]{
        // NOTE: We can't call printf here as it's not SMP safe

        // Test regular malloc
        {
          auto* m = malloc(0xffff);
          memset(m, 0, 0xffff);
          free(m);
        }

        // Test small std::array write/read on stack
        {
          std::array<std::byte, 1000> m;
          std::fill(m.begin(), m.end(), std::byte{8});

          for (auto j : m) {
            Expects(j == std::byte{8});
          }
        }

        // Test pmr vector
        {
          std::pmr::vector<std::byte> m;
          for (int j = 0; j < 0xffff; j++) {
            m.push_back(std::byte{32});
          }
        }

        barry.inc();


      }, i);

    SMP::signal(i);
  }
  // trigger interrupt
  SMP::signal();

  printf("Waiting for %i to exit...\n", SMP::cpu_count());
  barry.spin_wait(SMP::cpu_count());

  printf("SUCCESS\n");
}
