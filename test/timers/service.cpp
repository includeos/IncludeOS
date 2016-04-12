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
#include <stdio.h>
#include <vector>
#include <iostream>
#include <cassert>

using namespace std::chrono;

int one_shots = 0;
int repeat1 = 0;
int repeat2 = 0;

auto& timer = hw::PIT::instance();

void Service::start()
{


  INFO("Test Timers","Testing one-shot timers");


  int t1 = 0;
  int t10 = 0;
  int t2 = 0;

  // 30 sec. - Test End
  timer.onTimeout(30s, [] {
      printf("One-shots fired: %i \n", one_shots);
      CHECKSERT(one_shots == 5, "5 one-shot-timers fired");
      CHECKSERT(repeat1 == 25 and repeat2 == 10, "1s. timer fired 25 times, 2s. timer fired 10 times");
      CHECKSERT(timer.active_timers() == 1, "This is the last active timer");
      INFO("Test Timers","SUCCESS");
    });

  // 5 sec.
  timer.onTimeout(5s, [] {
      CHECKSERT(one_shots == 3,
                "After 5 sec, 3 other one-shot-timers have fired");
      one_shots++;
    });

  // 0.5 sec.
  timer.onTimeout(500ms, [] {
      CHECKSERT(one_shots == 0, "After 0.5 sec, no other one-shot-timers have fired");
      one_shots++;
    });

  // 1 sec.
  timer.onTimeout(1s, [] {
      CHECKSERT(one_shots == 1, "After 1 sec, 1 other one-shot-timers has fired");
      one_shots++;
    });


  // You can also use the std::function interface (which is great)
  std::vector<int> integers={1,2,3};

  std::function<void()> in_a_second = [integers](){
    for (auto i : integers)
      CHECKSERT(i == integers[i - 1], "%i == integers[%i - 1]", i, integers[i - 1]);
    one_shots++;
  };

  timer.onTimeout(1s, in_a_second);

  auto timer1s = timer.onRepeatedTimeout(1s, []{
      repeat1++;
      printf("1s. PULSE #%i \n", repeat1);
    });

  timer.onRepeatedTimeout(2s, []{
      repeat2++;
      printf("2s. PULSE #%i \n", repeat2);
    },

    // A continue-condition. The timer stops when false is returned
    []{
      CHECKSERT(repeat1 == (repeat2 * 2), "2s timer fired %i, 1s fired %i x 2 == %i times", repeat2, repeat2, repeat1);
      if (repeat2 >= 10) {
        CHECK(true, "2sec. pulse DONE!");
        return false;
      }
      return true;
    });


  // 25 sec. - end last repeating timer
  timer.onTimeout(25s + 10ms, [ timer1s ] {
      one_shots++;
      CHECKSERT(repeat1 == 25 and repeat2 == 10,
                "After 25 sec, 1s timer x 30 == %i times, 2s timer x 15 == %i times",
                repeat1, repeat2);

      timer.stop_timer(timer1s);
      CHECKSERT(timer.active_timers() == 2, "There are now 2 timers left");

      timer.onTimeout(1s, []{
          CHECKSERT(1, "Timers are still functioning");
        });

    });



}
