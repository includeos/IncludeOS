// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#include <os>
#include <net/inet4>

using namespace net;

void Service::start(const std::string&)
{
  auto& inet = Inet4::stack<0>();
  inet.network_config({  10,  0,  0, 45 },    // IP
                      { 255, 255, 0,  0 },    // Netmask
                      {  10,  0,  0,  1 } );  // Gateway
  printf("Service IP address is %s\n", inet.ip_addr().str().c_str());

  // ping gateway
  inet.icmp().ping(inet.gateway(), [](ICMP_packet pckt) {
    if (pckt)
      printf("Received packet from gateway\n%s\n", pckt.to_string().c_str());
    else
      printf("No reply received from gateway\n");
  });

  /* If IP forwarding on:
  // ping google.com with callback
  inet.icmp().ping(IP4::addr{193,90,147,109}, [](ICMP_packet pckt) {
    if (pckt.is_reply()) // or pckt.type() != icmp4::Type::NO_REPLY
      printf("Received packet\n%s\n", pckt.to_string().c_str());
    else
      printf("No reply received from 193.90.147.109. Identifier: %u. Sequence number: %u\n", pckt.id(), pckt.seq());
  });
  */

  // No reply-pings
  // Waiting 30 seconds for reply
  inet.icmp().ping(IP4::addr{10,0,0,42}, [](ICMP_packet pckt) {
    if (pckt)
      printf("Received packet from 10.0.0.42\n%s\n", pckt.to_string().c_str());
    else
      printf("No reply received from 10.0.0.42\n");
  });

  // Waiting 30 seconds for reply
  inet.icmp().ping(IP4::addr{10,0,0,43}, [](ICMP_packet pckt) {
    if (pckt)
      printf("Received packet from 10.0.0.43\n%s\n", pckt.to_string().c_str());
    else
      printf("No reply received from 10.0.0.43\n");
  });

  // Also possible without callback:
  // inet.icmp().ping(IP4::addr{193,90,147,109});  // google.com
}
