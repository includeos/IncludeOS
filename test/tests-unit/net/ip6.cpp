// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <map>
#include <common.cxx>
#include <nic_mock.hpp>
#include <net/inet>

#define MYINFO(X,...) INFO("Unit IP6", X, ##__VA_ARGS__)

using namespace net;
CASE("IP6 packet setters/getters")
{
  INFO("Unit", "IP6 packet setters/getters");
  SETUP("A pre-wired IP6 instance"){
    Nic_mock nic;
    Inet inet{nic};

      SECTION("Inet-created IP packets are initialized correctly") {
        auto ip_pckt1 = inet.create_ip6_packet(Protocol::ICMPv4);
        EXPECT(ip_pckt1->ip6_version() == 6);
        EXPECT(ip_pckt1->is_ipv6());
        EXPECT(ip_pckt1->ip_dscp() == DSCP::CS0);
        EXPECT(ip_pckt1->ip_ecn() == ECN::NOT_ECT);
        EXPECT(ip_pckt1->flow_label() == 0);
        EXPECT(ip_pckt1->payload_length() == 0);
        EXPECT(ip_pckt1->next_protocol() == Protocol::ICMPv4);
        EXPECT(ip_pckt1->hop_limit() == PacketIP6::DEFAULT_HOP_LIMIT);
        EXPECT(ip_pckt1->ip_src() == IP6::ADDR_ANY);
        EXPECT(ip_pckt1->ip_dst() == IP6::ADDR_ANY);
      }
  }
}
