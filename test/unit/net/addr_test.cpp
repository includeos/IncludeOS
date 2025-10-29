// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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
#include <net/addr.hpp>

using namespace net;

CASE("Default Addr intitialization")
{
  Addr addr;

  ip6::Addr ip6addr;
  EXPECT(addr.is_any());
  EXPECT(addr.is_v6());
  EXPECT_NOT(addr.is_v4());
  EXPECT(addr.v6() == ip6addr);
  EXPECT_THROWS(addr.v4());

  const std::string str{"0:0:0:0:0:0:0:0"};
  EXPECT(addr.to_string() == str);
}

CASE("Addr v4/v6")
{
  Addr addr{ip4::Addr::addr_any};

  EXPECT(addr.is_any());
  EXPECT(addr.is_v4());
  EXPECT_NOT(addr.is_v6());
  EXPECT(addr.v4() == ip4::Addr::addr_any);

  EXPECT(addr.to_string() == std::string("0.0.0.0"));

  const ip4::Addr ipv4{10,0,0,42};
  addr.set_v4(ipv4);
  EXPECT_NOT(addr.is_any());
  EXPECT(addr.is_v4());
  EXPECT(addr.v4() == ipv4);

  EXPECT(addr.to_string() == std::string("10.0.0.42"));

  const ip6::Addr ipv6{0,0,0x0000FFFF,ntohl(ipv4.whole)};
  EXPECT(addr.v6() == ipv6);
  EXPECT(addr.v6().to_string() == std::string("0:0:0:0:0:ffff:a00:2a"));

  addr.set_v6(ip6::Addr{0xfe80, 0, 0, 0, 0xe823, 0xfcff, 0xfef4, 0x85bd});
  EXPECT_NOT(addr.is_v4());
  EXPECT(addr.is_v6());
  EXPECT_NOT(addr.is_any());
  EXPECT(addr.to_string() == std::string("fe80:0:0:0:e823:fcff:fef4:85bd"));

}

CASE("v6 Linklocal/Solicit/Mcast")
{
  // Test me
}
