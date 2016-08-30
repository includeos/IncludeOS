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
#include <cassert>
#include <hw/pit.hpp>

using namespace std::chrono;

static int one_shots = 0;
static int repeat1 = 0;
static int repeat2 = 0;

void Service::start(const std::string&)
{
  INFO("Timers", "Testing one-shot timers");
  
  // test timer system
  extern void test_timers();
  test_timers();
  return;
  
  static auto& timer = hw::PIT::instance();

  // 30 sec. - Test End
  timer.on_timeout_ms(30s, [] {
      printf("One-shots fired: %i \n", one_shots);
      CHECKSERT(one_shots == 5, "5 one-shot-timers fired");
      CHECKSERT(repeat1 == 25 and repeat2 == 10, "1s. timer fired 25 times, 2s. timer fired 10 times");
      CHECKSERT(timer.active_timers() == 1, "This is the last active timer");
      INFO("Timers", "SUCCESS");
    });

  // 5 sec.
  timer.on_timeout_ms(5s, [] {
      printf("One-shots fired: %i \n", one_shots);
      CHECKSERT(one_shots == 3,
                "After 5 sec, 3 other one-shot-timers have fired");
      one_shots++;
    });

  // 0.5 sec.
  timer.on_timeout_ms(500ms, [] {
      CHECKSERT(one_shots == 0, "After 0.5 sec, no other one-shot-timers have fired");
      one_shots++;
    });

  // 1 sec.
  timer.on_timeout_d(1, [] {
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

  timer.on_timeout_ms(1s, in_a_second);

  auto timer1s = timer.on_repeated_timeout(1s, []{
      repeat1++;
      printf("1s. PULSE #%i \n", repeat1);
    });

  timer.on_repeated_timeout(2s, []{
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
  timer.on_timeout_ms(25s + 10ms, [ timer1s ] {
      one_shots++;
      CHECKSERT(repeat1 == 25 and repeat2 == 10,
                "After 25 sec, 1s timer x 30 == %i times, 2s timer x 15 == %i times",
                repeat1, repeat2);

      // Make sure this timer iterator is valid
      timer.stop_timer(timer1s);
      CHECKSERT(timer.active_timers() == 2, "There are now 2 timers left");

      timer.on_timeout_ms(1s, []{
          CHECKSERT(1, "Timers are still functioning");
        });

    });
}
