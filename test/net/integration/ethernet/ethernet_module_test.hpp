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

#ifndef ETHERNET_MODULE_TEST_HPP
#define ETHERNET_MODULE_TEST_HPP

#include <lest.hpp>
#include <net/inet4>

#define MYINFO(X,...) INFO("Ethernet Test",X,##__VA_ARGS__)

const lest::test ethernet_module_test[]
{
  {
    SCENARIO("Ethernet MAC address string representation")
    {
      GIVEN("A net::Ethernet::addr object")
      {
        const net::Ethernet::addr host_mac_address {0,240,34,255,45,11};

        WHEN("Converted to a std::string object")
        {
          auto mac_address_string = host_mac_address.str();

          THEN("The MAC address string representation must print leading zeros")
          {
            EXPECT(mac_address_string == "00:f0:22:ff:2d:0b");
          }
        }
      }
    }
  }
}; //< const lest::test ethernet_module_test[]

#endif //< ETHERNET_MODULE_TEST_HPP
