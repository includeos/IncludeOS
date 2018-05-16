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
#include <net/ip6/addr.hpp>

using namespace net::ip6;

CASE("Creating a empty IP6 address using the different constructors yields the same result")
{
  const std::string empty_addr_str {"0.0.0.0"};

  const Addr addr1;
  const Addr addr2 {0, 0, 0, 0};
  const Addr addr3 {0,0,0,0};

  EXPECT( addr1 == addr2 );
  EXPECT( addr2 == addr3 );
}

CASE("IP6 addresses can be compared to each other")
{
  Addr addr1 { 0xfe80, 0, 0, 0, 0xe823, 0xfcff, 0xfef4, 0x85bd };
  Addr addr2 { 0xfe80, 0, 0, 0, 0xe823, 0xfcff, 0xfef4, 0x84aa };
  Addr addr3 { };

  EXPECT_NOT( addr1 == addr2 );
  EXPECT_NOT( addr1 == addr3 );
  EXPECT( addr2 != addr3 );

  const Addr temp {  0xfe80, 0, 0, 0, 0xe823, 0xfcff, 0xfef4, 0x85bd };
  EXPECT( addr1 == temp );

  const Addr empty;
  EXPECT_NOT( addr1 == empty );

  EXPECT( addr1 > addr2 );
  EXPECT( addr1 > addr3 );
  EXPECT( addr2 > addr3 );
  EXPECT( addr3 < addr1 );
  EXPECT( addr2 <= addr1 );

  uint8_t netmask = 64; 
  const Addr not_terrorist { 0xfe80, 0, 0, 0, 0xe823, 0xfcff, 0xfef4, 0x85bd };

  Addr result = not_terrorist & netmask;
  const Addr expected_result { 0xfe80, 0, 0, 0, 0, 0, 0, 0 };
  EXPECT( result == expected_result );
}

CASE("Determine if an address is loopback")
{
  Addr l1 { 0,0,0,1 };
  Addr l2 { 127,10,0,42 };
  Addr l3 { 0xfe80, 0, 0, 0, 0xe823, 0xfcff, 0xfef4, 0x85bd };

  EXPECT(l1.is_loopback());
  EXPECT(not l2.is_loopback());
  EXPECT(not l3.is_loopback());
}

CASE("Determine if an address is a multicast")
{
  Addr l1 { 0xffffffff,0,0,1 };
  Addr l2 { 0xff000000,0,0,0};
  Addr l3 { 0xfe80, 0, 0, 0, 0xe823, 0xfcff, 0xfef4, 0x85bd };

  EXPECT(l1.is_multicast());
  EXPECT(l2.is_multicast());
  EXPECT(not l3.is_multicast());
}
