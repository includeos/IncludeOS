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

#ifndef IPV4_MODULE_TEST_HPP
#define IPV4_MODULE_TEST_HPP

#include <lest.hpp>
#include <net/inet4>

#define MYINFO(X,...) INFO("IPv4 Test",X,##__VA_ARGS__)


const lest::test ipv4_module_test[]
{
  {
    SCENARIO("IPv4 address construction")
    {
      GIVEN("A valid IPv4 representation")
      {
        const std::string ipv4_string {"192.168.0.1"};
        net::IP4::addr ipv4_address {ipv4_string};
        EXPECT(ipv4_address.str() == ipv4_string);
      }

      GIVEN("A valid IPv4 representation with extraneous whitespace")
      {
        const std::string ipv4_string {"\r 10.0.0.42 \n\r"};
        net::IP4::addr ipv4_address {ipv4_string};
        EXPECT(ipv4_address.str() == "10.0.0.42");
      }

      GIVEN("A std::string object representing an invalid IPv4 address")
      {
        const std::string ipv4_address {"256.652.300.4"};
        EXPECT_THROWS(net::IP4::addr host_address {ipv4_address});
      }
      GIVEN("A std::string object representing an invalid IPv4 address")
      {
        const std::string ipv4_address {"jklasdfølkj asdfløkj aløskdjf ølkajs dfølkj asdf"};
        EXPECT_THROWS(net::IP4::addr host_address {ipv4_address});
      }
    }
  }
}; //< const lest::test ipv4_module_test[]

#endif //< IPV4_MODULE_TEST_HPP
