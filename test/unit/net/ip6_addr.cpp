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
  const std::string empty_addr_str {"0:0:0:0:0:0:0:0"};

  const Addr addr1;
  const Addr addr2 {0, 0, 0, 0};
  const Addr addr3 {0,0,0,0};
  const Addr addr4 {"::"};
  const Addr addr5 {empty_addr_str};


  EXPECT( addr1 == addr2 );
  EXPECT( addr2 == addr3 );
  EXPECT( addr3 == addr4 );
  EXPECT( addr4 == addr5 );

  EXPECT( addr1.str() == empty_addr_str);
}

CASE("Create IP6 addresses from strings")
{
  Addr addr1 { 0xfe80, 0, 0, 0, 0xe823, 0xfcff, 0xfef4, 0x85bd };
  Addr addr2 { 0xfe80, 0, 0, 0, 0xe823, 0xfcff, 0xfef4, 0x84aa };
  Addr addr3 { };
  //lowercase test
  std::string valid_addr_string1{"fe80:0000:0000:0000:e823:fcff:fef4:85bd"};
  std::string valid_addr_short_string1{"fe80::e823:fcff:fef4:85bd"};
  //uppercase test
  std::string valid_addr_string2 {"FE80:0000:0000:0000:E823:FCFF:FEF4:84AA"};
  std::string valid_addr_short_string2 {"FE80::E823:FCFF:FEF4:84AA"};

  Addr valid_addr1{valid_addr_string1};
  Addr valid_addr2{valid_addr_string2};
  //Uppercase and lowercase validation
  EXPECT (valid_addr1 == addr1);
  EXPECT (valid_addr2 == addr2);

  EXPECT (Addr{valid_addr_short_string1}== addr1);
  EXPECT (Addr{valid_addr_short_string2} == addr2);

  std::string invalid_addr_string1 {"CAFEBABE::e823:fcff:fef4::85bd"};
  EXPECT_THROWS(Addr{invalid_addr_string1});

  //edge cases
  EXPECT(Addr{"FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF"} == Addr(0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff));
  EXPECT(Addr{"0000:0000:0000:0000:0000:0000:0000:0000"} == Addr(0,0,0,0,0,0,0,0));
  EXPECT(Addr{"::"} == Addr(0,0,0,0,0,0,0,0));
  EXPECT(Addr{"FFFF::"} == Addr(0xffff,0,0,0,0,0,0,0));
  EXPECT(Addr{"::FFFF"} == Addr(0,0,0,0,0,0,0,0xffff));
  EXPECT(Addr{"0::"} == Addr(0,0,0,0,0,0,0,0));
  EXPECT(Addr{"::0"} == Addr(0,0,0,0,0,0,0,0));
  EXPECT(Addr{"0::0"} == Addr(0,0,0,0,0,0,0,0));
  EXPECT(Addr{"1::1"} == Addr(1,0,0,0,0,0,0,1));
  EXPECT(Addr{"FE80::"}==Addr(0xfe80,0,0,0,0,0,0,0));
  EXPECT(Addr{"FE80::1"}==Addr(0xfe80,0,0,0,0,0,0,1));

  //end expansion to 0
  EXPECT(Addr{"1::"}==Addr(1,0,0,0,0,0,0,0));
  EXPECT(Addr{"1:2::"}==Addr(1,2,0,0,0,0,0,0));
  EXPECT(Addr{"1:2:3::"}==Addr(1,2,3,0,0,0,0,0));
  EXPECT(Addr{"1:2:3:4::"}==Addr(1,2,3,4,0,0,0,0));
  EXPECT(Addr{"1:2:3:4:5::"}==Addr(1,2,3,4,5,0,0,0));
  EXPECT(Addr{"1:2:3:4:5:6::"}==Addr(1,2,3,4,5,6,0,0));
  //not end expansion
  EXPECT(Addr{"1::8"}==Addr(1,0,0,0,0,0,0,8));
  EXPECT(Addr{"1:2::8"}==Addr(1,2,0,0,0,0,0,8));
  EXPECT(Addr{"1:2:3::8"}==Addr(1,2,3,0,0,0,0,8));
  EXPECT(Addr{"1:2:3:4::8"}==Addr(1,2,3,4,0,0,0,8));
  EXPECT(Addr{"1:2:3:4:5::8"}==Addr(1,2,3,4,5,0,0,8));
  EXPECT(Addr{"1:2:3:4:5:6::8"}==Addr(1,2,3,4,5,6,0,8));
  //start zero expansion
  EXPECT(Addr{"::8"}==Addr(0,0,0,0,0,0,0,8));
  EXPECT(Addr{"::7:8"}==Addr(0,0,0,0,0,0,7,8));
  EXPECT(Addr{"::6:7:8"}==Addr(0,0,0,0,0,6,7,8));
  EXPECT(Addr{"::5:6:7:8"}==Addr(0,0,0,0,5,6,7,8));
  EXPECT(Addr{"::4:5:6:7:8"}==Addr(0,0,0,4,5,6,7,8));
  EXPECT(Addr{"::3:4:5:6:7:8"}==Addr(0,0,3,4,5,6,7,8));
  //not start expansion
  EXPECT(Addr{"1::8"}==Addr(1,0,0,0,0,0,0,8));
  EXPECT(Addr{"1::7:8"}==Addr(1,0,0,0,0,0,7,8));
  EXPECT(Addr{"1::6:7:8"}==Addr(1,0,0,0,0,6,7,8));
  EXPECT(Addr{"1::5:6:7:8"}==Addr(1,0,0,0,5,6,7,8));
  EXPECT(Addr{"1::4:5:6:7:8"}==Addr(1,0,0,4,5,6,7,8));
  EXPECT(Addr{"1::3:4:5:6:7:8"}==Addr(1,0,3,4,5,6,7,8));


  //error cases

  //to many :
  EXPECT_THROWS(Addr{"1:2:3:4:5:6:7::"}==Addr(1,2,3,4,5,6,7,0));
  EXPECT_THROWS(Addr{"::2:3:4:5:6:7:8"}==Addr(0,2,3,4,5,6,7,8));

  //illegal character
  EXPECT_THROWS(Addr{"whe:re:is:my:love::"});
  //to many characters in quibble
  EXPECT_THROWS(Addr{"FF01::CAFEBABE"});
  //double ::
  EXPECT_THROWS(Addr{"FF01::CAFE::BABE"});
  //to many :
  EXPECT_THROWS(Addr{"1:2:3:4:5:6:7:8:9"});
  //to few :
  EXPECT_THROWS(Addr{"1:2:3:4:5:6:7"});
  //to many characters in total
  EXPECT_THROWS(Addr{"FF01:FF02:FF02:FF02:FF02:FF02:FF02:FF021"});
  //to few characters in total
  EXPECT_THROWS(Addr{":"});
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
