// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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
#include <kernel/timers.hpp>
using namespace std::chrono;

extern delegate<uint64_t()> systime_override;
static uint64_t current_time = 0;

static int magic_performed = 0;
void perform_magic(int) {
  magic_performed += 1;
}

CASE("Initialize timer system")
{
  systime_override =
    [] () -> uint64_t { return current_time; };

  Timers::init(
    [] (Timers::duration_t) {},
    [] () {}
  );
  Timers::ready();

  EXPECT(Timers::is_ready());
  EXPECT(Timers::active() == 0);
  EXPECT(Timers::existing() == 0);
  EXPECT(Timers::free() == 0);
}

CASE("Start a single timer, execute it")
{
  current_time = 0;
  magic_performed = 0;
  // start timer
  Timers::oneshot(1ms, perform_magic);
  EXPECT(Timers::active() == 1);
  EXPECT(Timers::existing() == 1);
  EXPECT(Timers::free() == 0);
  // execute timer interrupt
  Timers::timers_handler();
  // verify timer did not execute
  EXPECT(magic_performed == 0);
  // set time to where it should happen
  current_time = 1000000;
  // execute timer interrupt
  Timers::timers_handler();
  // verify timer did execute
  EXPECT(magic_performed == 1);
}

CASE("Start a single timer, execute it precisely")
{
  current_time = 0;
  magic_performed = 0;
  // start timer
  Timers::oneshot(1ms, perform_magic);
  // verify timer did not execute for all times before 1ms
  for (uint64_t time = 0; time < 1000000; time += 1000)
  {
    Timers::timers_handler();
    EXPECT(magic_performed == 0);
  }
  // set time to where it should happen
  current_time = 1000000;
  // execute timer interrupt
  Timers::timers_handler();
  // verify timer did execute
  EXPECT(magic_performed == 1);
}

CASE("Start many timers, execute all at once")
{
  current_time = 0;
  magic_performed = 0;
  // start many timers, starting at 1ms and ending at 2ms
  for (int i = 0; i < 1000; i++)
    Timers::oneshot(microseconds(1000 + i), perform_magic);
  // verify timer did not execute for all times before 1ms
  for (uint64_t time = 0; time < 1000000; time += 1000)
  {
    Timers::timers_handler();
    EXPECT(magic_performed == 0);
  }
  current_time = 2000000;
  // verify many timers executed
  Timers::timers_handler();
  EXPECT(magic_performed == 1000);
  EXPECT(Timers::active()   == 0);
  EXPECT(Timers::existing() == 1000);
  EXPECT(Timers::free()     == 1000);
  // restore time
  current_time = 0;
}

CASE("Catch some Timers exceptions")
{
  EXPECT_THROWS(Timers::stop(-1));
}

CASE("Stop a timer")
{
  current_time = 0;
  magic_performed = 0;
  // start timer
  int id = Timers::oneshot(1ms, perform_magic);
  EXPECT(Timers::active() == 1);
  // execute timer interrupt
  Timers::timers_handler();
  // verify timer did not execute
  EXPECT(magic_performed == 0);
  // set time to where it should happen
  current_time = 1000000;
  // stop timer
  Timers::stop(id);
  // execute timer interrupt
  Timers::timers_handler();
  // verify timer did not execute, since it was stopped
  EXPECT(magic_performed == 0);
}

CASE("Test a periodic timer")
{
  current_time = 0;
  magic_performed = 0;
  // start timer
  int id = Timers::periodic(microseconds(1), perform_magic);
  EXPECT(Timers::active() == 1);

  for (int i = 0; i < 100000; i += 1000)
  {
    current_time = i;
    // execute timer interrupt
    Timers::timers_handler();
    // verify timer did not execute
    EXPECT(magic_performed == i / 1000);
  }
  // stop timer
  Timers::stop(id);
  current_time = 0;
}

#include <util/timer.hpp>
CASE("Test util timer")
{
  Timer timer{ [] () {} };
  EXPECT(!timer.is_running());
  timer.start( std::chrono::milliseconds(1), [] () {});
  EXPECT(timer.is_running());
  timer.restart( std::chrono::milliseconds(1), [] () {});
  EXPECT(timer.is_running());
  timer.stop();
  EXPECT(!timer.is_running());
}