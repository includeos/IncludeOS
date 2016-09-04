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

#define GSL_THROW_ON_CONTRACT_VIOLATION 1

#include <common.cxx>
#include <net/ip4/addr.hpp>

using namespace net::ip4;

CASE("Creating a empty IP4 address using the different constructors yields the same result")
{
  const std::string empty_addr_str {"0.0.0.0"};

  const Addr addr1;
  const Addr addr2 {0};
  const Addr addr3 {0,0,0,0};
  const Addr addr4 {empty_addr_str};

  EXPECT( addr1 == 0 );
  EXPECT( addr2 == 0 );
  EXPECT( addr3 == 0 );

  EXPECT( addr1.to_string() == empty_addr_str );

  EXPECT( addr1 == addr2 );
  EXPECT( addr2 == addr3 );
  EXPECT( addr3 == addr4 );
}

CASE("Create IP4 addresses from strings")
{
  const Addr addr {10,0,0,42};
  std::string valid_addr_str {"10.0.0.42"};
  const Addr valid_addr{valid_addr_str};
  EXPECT( valid_addr == addr );
  EXPECT( valid_addr.to_string() == valid_addr_str );

  EXPECT_THROWS(Addr{"LUL"});
  EXPECT_THROWS(Addr{"12310298310298301283"});
  EXPECT_THROWS(const Addr invalid{"256.256.256.256"});
}

CASE("IP4 addresses can be compared to each other")
{
  Addr addr1 { 10,0,0,42 };
  Addr addr2 { 10,0,0,43 };
  Addr addr3 { 192,168,0,1 };

  EXPECT_NOT( addr1 == addr2 );
  EXPECT_NOT( addr1 == addr3 );
  EXPECT( addr2 != addr3 );

  const Addr temp { 10,0,0,42 };
  EXPECT( addr1 == temp );

  const Addr empty;
  EXPECT_NOT( addr1 == empty );

  EXPECT( addr1 < addr2 );
  EXPECT( addr1 < addr3 );
  EXPECT( addr2 < addr3 );
  EXPECT( addr3 > addr1 );

  const Addr netmask { 255,255,255,0 };
  const Addr not_terrorist { 192,168,1,14 };

  Addr result = netmask & not_terrorist;
  const Addr expected_result { 192,168,1,0 };
  EXPECT( result == expected_result );

}
