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
#include <kernel/events.hpp>
const int Events::NUM_EVENTS;

static inline auto& manager() {
  return Events::get(0);
}

CASE("Call process events without any subscriptions")
{
  // should not be processing any events
  manager().process_events();
}

CASE("Verify integrity of subscription list")
{
  static int called_before = 0;
  static int called_times = 0;

  for (int i = 0; i < 1000; i++)
  {
    // first simple event
    auto first = manager().subscribe(
    [] { called_times++; });
    // second more complex event
    auto second = manager().subscribe(
    [first] {
      called_times++;
      // create third event which self-unsubs
      manager().defer(
        [] {
          called_times++;
        });
      // unsub from first event
      manager().unsubscribe(first);
    });
    // call first and second event
    manager().trigger_event(first);
    manager().trigger_event(second);
    // process all
    manager().process_events();
    EXPECT(called_times == called_before + 3);
    called_before = called_times;
    // free event
    manager().unsubscribe(second);
  }
}

CASE("Subscribe to each event, call, verify")
{
  static std::vector<bool> called(Events::NUM_EVENTS);
  static int called_times = 0;

  for (int i = 0; i < Events::NUM_EVENTS; i++)
  {
    EXPECT(called[i] == false);
    manager().subscribe(i,
    [i] ()
    {
      called[i] = true;
      called_times++;
    });
    // event should still not be called
    EXPECT(called[i] == false);
    EXPECT(called_times == i);
    // soft-trigger event
    manager().trigger_event(i);
    // event should still not be called
    EXPECT(called[i] == false);
    EXPECT(called_times == i);
    // process events
    manager().process_events();
    // event should have been called
    EXPECT(called[i] == true);
    EXPECT(called_times == i + 1);
  }
  // re-trigger all events
  called_times = 0;
  for (int i = 0; i < Events::NUM_EVENTS; i++)
  {
    // soft-trigger event
    manager().trigger_event(i);
  }
  // process all events
  manager().process_events();
  // all event should have been called
  for (auto v : called) EXPECT(v == true);
  EXPECT(called_times == Events::NUM_EVENTS);
  for (int i = 0; i < Events::NUM_EVENTS; i++)
  {
    manager().unsubscribe(i);
  }
}

CASE("Too many events throws exception")
{
  // subscribe to all events
  for (int i = 0; i < Events::NUM_EVENTS; i++)
  {
    manager().subscribe(i, [] { });
  }
  EXPECT_THROWS(manager().subscribe([] { }));
  // free all events again
  for (int i = 0; i < Events::NUM_EVENTS; i++)
  {
    manager().unsubscribe(i);
  }
  // event not subscribed on should throw
  EXPECT_THROWS(manager().unsubscribe(35));
}
