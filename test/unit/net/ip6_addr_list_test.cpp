// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2019 Oslo and Akershus University College of Applied Sciences
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
#include <net/ip6/addr_list.hpp>

static uint64_t my_time = 0;

static uint64_t get_time()
{ return my_time; }

#include <delegate>
extern delegate<uint64_t()> systime_override;

using namespace net;
using namespace net::ip6;

CASE("IP6 Addr_list static test")
{
  Addr_list list;
  int res = 0;

  const ip6::Addr local{"fe80::1337:1337"};
  const ip6::Addr global{"2001:1337:1337:1337::1337"};

  const ip6::Addr local_dest{"fe80::4242:1"};
  const ip6::Addr global_dest{"2001:1338:1338:1338::1337"};

  EXPECT(list.empty());

  res = list.input(local, 64, Stateful_addr::infinite_lifetime, Stateful_addr::infinite_lifetime);
  EXPECT(res == 1);
  EXPECT(not list.empty());
  EXPECT(list.has(local));
  EXPECT(not list.has(global));

  res = list.input(global, 64, Stateful_addr::infinite_lifetime, Stateful_addr::infinite_lifetime);
  EXPECT(res == 1);
  EXPECT(list.has(global));

  EXPECT(list.get_src(local_dest) == local);
  EXPECT(list.get_src(global_dest) == global);

  res = list.input(local, 64, 0, 0);
  EXPECT(res == -1);
  EXPECT(not list.has(local));
  EXPECT(list.get_src(local_dest) == ip6::Addr::addr_any);

  res = list.input(global, 64, 0, 0);
  EXPECT(res == -1);
  EXPECT(not list.has(global));
  EXPECT(list.get_src(global_dest) == ip6::Addr::addr_any);

  EXPECT(list.empty());
}

CASE("IP6 Addr_list autoconf test")
{
  // setup RTC to use my_time
  systime_override = get_time;
  my_time = 0;

  Addr_list list;
  int res = 0;

  const ip6::Addr local{"fe80::1337:1337"};
  const ip6::Addr global{"2001:1337:1337:1337::1337"};

  const ip6::Addr local_dest{"fe80::4242:1"};
  const ip6::Addr global_dest{"2001:1338:1338:1338::1337"};

  res = list.input_autoconf(local, 64, 3000, 6000);
  EXPECT(res == 1);
  EXPECT(not list.empty());
  EXPECT(list.has(local));
  EXPECT(not list.has(global));
}
