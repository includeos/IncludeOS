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

/**
 * @brief This is defined to quiet the compiler from complaining
 * about the missing symbol {clock_gettime}
 */
int clock_gettime(clockid_t clk_id, struct timespec *tp){
  (void*)clk_id;
  (void*)tp;
  return 0;
};

const lest::test ipv4_module_test[]
{
  {
    SCENARIO("IPv4 address construction")
    {
      GIVEN("A std::string object representing a valid IPv4 address")
      {
        const std::string ipv4_address {"10.0.0.42"};

        WHEN("Passed to the net::IPv4::addr constructor")
        {
          net::IP4::addr host_address {ipv4_address};

          THEN("The net::IP4::addr object must reflect the given address")
          {
            EXPECT(host_address.str() == ipv4_address);
          }
        }
      }

      GIVEN("A std::string object representing an invalid IPv4 address")
      {
        const std::string ipv4_address {"256.652.300.4"};

        WHEN("Passed to the net::IPv4::addr constructor")
        {
          net::IP4::addr host_address {ipv4_address};

          THEN("The net::IP4::addr object must reflect the address 0.0.0.0")
          {
            EXPECT(host_address.str() == "0.0.0.0");
          }
        }
      }
    }
  }
}; //< const lest::test ipv4_module_test[]

#endif //< IPV4_MODULE_TEST_HPP
