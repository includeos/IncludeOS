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

  std::string extra_whitespace_addr_str {"\r 10.0.0.42 \n\r"};
  EXPECT_THROWS(Addr{extra_whitespace_addr_str});
  EXPECT(Addr{"10"} == Addr(10,0,0,0));
  EXPECT_THROWS(Addr{"10."});
  EXPECT(Addr{"10.0"} == Addr(10,0,0,0));
  EXPECT_THROWS(Addr{"10.0."});
  EXPECT(Addr{"10.0.0"} == Addr(10,0,0,0));
  EXPECT_THROWS(Addr{"10.0.0."});

  // Check some special cases / previously failed cases
  std::vector<std::string> ips {
    "0.0.0.0",
    "255.255.255.255",
    "1.2.3.4",
    "11.22.33.44",
    "111.222.255.255",
    "195.22.13.24",
    "195.255.13.24",
    "196.209.13.24",
    "197.111.13.24",
    "198.211.13.24",
    "199.210.13.24",
    "200.220.13.24",
    "201.230.13.24",
    "202.240.13.24",
    "203.250.13.24",
    "204.251.13.24"};

  for (auto& str : ips){
    Addr ipv4_address2 {str};
    EXPECT(ipv4_address2.str() == str);
  }

  // Check a number of random IP's
  for (uint32_t i = 0; i<1000; i++){
    uint32_t r = (uint32_t)rand();
    Addr ip1{r};
    Addr ip2{ip1.to_string()};
    Expects(ip1.whole == r);
    Expects(ip2.whole == r);
    Expects(ip1 == ip2);
  }

  /** When in doubt, check all of them
  uint32_t max = std::numeric_limits<uint32_t>::max();

  for (uint32_t i = 0;i < max; i++) {
    Addr ip{i};
    Expects(ip.whole == i);
    Expects(Addr{ip.to_string()}.whole == i);
  }
  **/

  EXPECT_NO_THROW(Addr("202.209.27.78"));
  EXPECT_NO_THROW(Addr("212.209.27.78"));
  EXPECT_NO_THROW(Addr("222.209.27.78"));
  EXPECT_NO_THROW(Addr("232.209.27.78"));
  EXPECT_NO_THROW(Addr("242.209.27.78"));
  EXPECT_NO_THROW(Addr("255.209.27.78"));
  EXPECT_THROWS(Addr("265.209.27.78"));
  EXPECT_THROWS(Addr("256.209.27.78"));
  EXPECT_THROWS(Addr{"LUL"});
  EXPECT_THROWS(Addr{"12310298310298301283"});
  EXPECT_THROWS(const Addr invalid1{"256.256.256.256"});
  EXPECT_THROWS(const Addr invalid2{"1.1.1.256"});
  EXPECT_THROWS(const Addr invalid3{"999.99.9.9"});
  EXPECT_THROWS(const Addr invalid4{"-6.-2.-5.1"});
  EXPECT_THROWS(const Addr invalid4{"1.2.3.4.5"});
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

CASE("Determine if an address is loopback")
{
  Addr l1 { 127,0,0,1 };
  Addr l2 { 127,10,0,42 };
  Addr l3 { 127,255,255,255 };
  Addr no { 128,0,0,2 };

  EXPECT(l1.is_loopback());
  EXPECT(l2.is_loopback());
  EXPECT(l3.is_loopback());
  EXPECT(not no.is_loopback());

}
