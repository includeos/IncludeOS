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
#include <net/ip6/stateful_addr.hpp>

static uint64_t my_time = 0;

static uint64_t get_time()
{ return my_time; }

#include <delegate>
extern delegate<uint64_t()> systime_override;

using namespace net::ip6;

CASE("Stateful_addr basic and lifetime")
{
  // setup RTC to use my_time
  systime_override = get_time;
  my_time = 0;

  const Addr local{"fe80::1337:1"};
  const uint8_t prefix = 64;

  Stateful_addr addr{local, prefix, 2400, 3600};

  EXPECT(addr.addr() == local);
  EXPECT(addr.prefix() == prefix);
  EXPECT(addr.valid());
  EXPECT(addr.preferred());
  EXPECT(not addr.always_valid());
  EXPECT(addr.remaining_valid_time() == 3600);
  EXPECT(addr.valid_ts() == 3600);
  EXPECT(addr.preferred_ts() == 2400);

  my_time += 2400;

  EXPECT(addr.valid());
  EXPECT(not addr.preferred()); // deprecated
  EXPECT(addr.remaining_valid_time() == 3600-2400);

  my_time += 1200;

  EXPECT(not addr.valid());
  EXPECT(not addr.preferred());
  EXPECT(addr.remaining_valid_time() == 0);

  my_time += 1200;

  EXPECT(not addr.valid());
  EXPECT(not addr.preferred());
  EXPECT(addr.remaining_valid_time() == 0);
  EXPECT(addr.valid_ts() == 3600);
  EXPECT(addr.preferred_ts() == 2400);

  addr.update_valid_lifetime(3600);

  EXPECT(addr.valid());
  EXPECT(not addr.preferred());
  EXPECT(addr.valid_ts() == 3600 + my_time);

  addr.update_preferred_lifetime(3600);

  EXPECT(addr.valid());
  EXPECT(addr.preferred());
  EXPECT(addr.preferred_ts() == 3600 + my_time);

  addr.update_valid_lifetime(0);

  EXPECT(not addr.valid());
  EXPECT(not addr.preferred());
  EXPECT(addr.valid_ts() == my_time);

  my_time += 1200;

  EXPECT(addr.remaining_valid_time() == 0);

  EXPECT(addr.to_string() == std::string("fe80:0:0:0:0:0:1337:1/64"));

}

CASE("Stateful_addr infinite lifetime")
{
  // setup RTC to use my_time
  systime_override = get_time;
  my_time = 0;

  const Addr local{"fe80::1337:1"};
  const uint8_t prefix = 64;

  Stateful_addr addr{local, prefix};

  EXPECT(addr.always_valid());
  EXPECT(addr.valid());
  EXPECT(addr.preferred());
  EXPECT(addr.remaining_valid_time() == Stateful_addr::infinite_lifetime);
  EXPECT(addr.valid_ts() == Stateful_addr::infinite_lifetime);
  EXPECT(addr.preferred_ts() == Stateful_addr::infinite_lifetime);

  my_time += 6000;

  EXPECT(addr.valid());
  EXPECT(addr.preferred());
  EXPECT(addr.remaining_valid_time() == Stateful_addr::infinite_lifetime);

  addr.update_valid_lifetime(0);
  EXPECT(not addr.valid());
  EXPECT(not addr.always_valid());
  EXPECT(addr.valid_ts() == my_time);
}

CASE("Stateful_addr matches")
{
  const Stateful_addr addr1{{"fe80::1337:0:0:1337"}, 64};
  const Stateful_addr addr2{{"fe80::1337:0:42:1337"}, 64};
  const Stateful_addr router1{{"fe80::1337:1337:0:0:1"}, 48};
  const Stateful_addr router2{{"fe80::1337:0:0:1"}, 112};

  EXPECT(router1.match(addr1.addr()));
  EXPECT(router1.match(addr2.addr()));
  EXPECT(router2.match(addr1.addr()));
  EXPECT(not router2.match(addr2.addr()));
}
