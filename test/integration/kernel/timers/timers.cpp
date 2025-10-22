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
#include <cstdio>
#include <cassert>
#include <timers>
#include <vector>

using namespace std::chrono;

static int one_shots = 0;
static int repeat1 = 0;
static int repeat2 = 0;

void test_timers()
{
  INFO("Timers", "Testing kernel timers");
  // a calibration timer is active on bare metal and in emulated environments
  static size_t BASE_TIMERS;
  BASE_TIMERS = Timers::active();

  // 30 sec. - Test End
  Timers::oneshot(30s, [] (auto) {
      printf("One-shots fired: %i \n", one_shots);
      CHECKSERT(one_shots == 5, "5 one-shot-timers fired");
      CHECKSERT(repeat1 == 25 and repeat2 == 10, "1s. timer fired 25 times, 2s. timer fired 10 times");
      CHECKSERT(Timers::active() == BASE_TIMERS+0, "No more active timers");
      INFO("Timers", "SUCCESS");
    });

  // 5 sec.
  Timers::oneshot(5s, [] (auto) {
      printf("One-shots fired: %i \n", one_shots);
      CHECKSERT(one_shots == 3,
                "After 5 sec, 3 other one-shot-timers have fired");
      one_shots++;
    });

  // 0.5 sec.
  Timers::oneshot(500ms, [] (auto) {
      CHECKSERT(one_shots == 0, "After 0.5 sec, no other one-shot-timers have fired");
      one_shots++;
    });

  // 1 sec.
  Timers::oneshot(1s, [] (auto) {
      CHECKSERT(one_shots == 1, "After 1 sec, 1 other one-shot-timers has fired");
      one_shots++;
    });


  // You can also use the std::function interface (which is great)
  std::vector<int> integers={1,2,3};

  auto in_a_second =
  [integers] (Timers::id_t) {
    for (auto i : integers)
      CHECKSERT(i == integers[i - 1], "%i == integers[%i - 1]", i, integers[i - 1]);
    one_shots++;
  };

  Timers::oneshot(1s, in_a_second);

  auto timer1s = Timers::periodic(1s, 1s,
  [] (auto) {
      repeat1++;
      printf("1s. PULSE #%i \n", repeat1);
    });

  Timers::periodic(2s, 2s,
  [] (auto id) {
      repeat2++;
      printf("2s. PULSE #%i \n", repeat2);

      //CHECKSERT(repeat1 == (repeat2 * 2), "2s timer fired %i, 1s fired %i x 2 == %i times", repeat2, repeat2, repeat1);
      if (repeat2 >= 10) {
        CHECK(true, "2sec. pulse DONE!");
        Timers::stop(id);
      }
    });

  // 25 sec. - end last repeating timer
  Timers::oneshot(25s + 20ms,
  [timer1s] (auto) {
      one_shots++;
      printf("repeat1: %u,  repeat2: %u\n", repeat1, repeat2);
      CHECKSERT(repeat1 == 25 and repeat2 == 10,
                "After 25 sec, 1s timer x 30 == %i times, 2s timer x 15 == %i times",
                repeat1, repeat2);

      // Make sure this timer iterator is valid
      Timers::stop(timer1s);
      // The current timer does not count towards the total active
      CHECKSERT(Timers::active() == BASE_TIMERS+1, "Only the last finish timer is left");

      Timers::oneshot(1s,
      [] (auto) {
        CHECKSERT(1, "Timers are still functioning");
      });
    });
}
