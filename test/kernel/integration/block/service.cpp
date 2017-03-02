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
#include <timers>

extern "C" uint32_t os_get_blocking_level();
extern "C" uint32_t os_get_highest_blocking_level();

void sleep(int i){
  static Timers::id_t timer{};

  if (not timer)
    timer = Timers::oneshot(std::chrono::seconds(i), [](auto){
        INFO("Timer", "Ticked");
        timer = 0;
        Expects(os_get_blocking_level() == 1);
      });

  while (timer) {
    OS::block();
    Expects(os_get_blocking_level() == 0);
  }

  INFO("Test", " Done");
}

void Service::start()
{
  INFO("Block", "Testing blocking calls.");
}

void Service::ready()
{
  static int sleeps = 0;

  Timers::oneshot(std::chrono::seconds(5), [](auto){
      INFO("Test","About half way done (%i / 10)", sleeps);
      Expects(sleeps < 10);
    });

  Timers::oneshot(std::chrono::seconds(20), [](auto){
      Expects(sleeps >= 10);
    });

  int n = 10;
  for (int i = 0; i < n; i++) {
    sleep(1);
    sleeps++;
    printf("Sleep %i/%i \n", sleeps, n);
    if (sleeps == 10)
    INFO("Test", "SUCCESS");
  }

  Expects(os_get_blocking_level() == 0);
  Expects(os_get_highest_blocking_level() == 1);
}
