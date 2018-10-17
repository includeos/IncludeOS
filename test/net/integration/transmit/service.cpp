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

//#define DEBUG // Debug supression

#include <service>
#include <net/interfaces>

using namespace std;
using namespace net;
auto& timer = hw::PIT::instance();

void Service::start(const std::string&)
{
  static auto& inet = Interfaces::get(0);
  inet.network_config(
         { 10,0,0,49 },     // IP
         { 255,255,255,0 }, // Netmask
         { 10,0,0,1 },      // Gateway
         { 8,8,8,8 });      // DNS

  printf("Service IP address is %s\n", inet.ip_addr().str().c_str());

  const UDP::port_t port = 4242;
  auto& conn = inet.udp().bind(port);
  conn.on_read(
  [&conn] (auto addr, auto port, const char* data, int len) {
    auto received = std::string(data, len);

    if (received == "SUCCESS") {
      INFO("Test 2", "SUCCESS");
      return;
    }

    INFO("Test 2","Starting UDP-test. Got UDP data from %s: %i: %s",
         addr.str().c_str(), port, received.c_str());

    const size_t PACKETS = 600;
    const size_t PAYLOAD_LEN = inet.ip_obj().MDDS() - sizeof(UDP::udp_header);

    std::string first_reply = to_string(PACKETS * PAYLOAD_LEN);

    // Send the first packet, and then wait for ARP
    conn.sendto(addr, port, first_reply.data(), first_reply.size());

    timer.on_timeout_ms(200ms,
    [&conn, addr, port, PACKETS, PAYLOAD_LEN] {

      INFO("Test 2", "Trying to transmit %u UDP packets of size %u at maximum throttle",
           PACKETS, PAYLOAD_LEN);

      for (size_t i = 0; i < PACKETS*2; i++) {
        const char c = 'A' + (i % 26);
        std::string send(PAYLOAD_LEN, c);
        conn.sendto(addr, port, send.data(), send.size());
      }

      CHECK(1,"UDP-transmission didn't panic");
      INFO("UDP Transmision tests", "OK");
    });
  });

  timer.on_timeout_ms(2ms,
  [=] {
    const int PACKETS = 600;
    INFO("Test 1", "Trying to transmit %i ethernet packets at maximum throttle", PACKETS);
    for (int i=0; i < PACKETS; i++){
      auto pckt = inet.create_packet(inet.MTU());
      Ethernet::header* hdr = reinterpret_cast<Ethernet::header*>(pckt->buffer());
      hdr->dest.major = Ethernet::BROADCAST_FRAME.major;
      hdr->dest.minor = Ethernet::BROADCAST_FRAME.minor;
      hdr->type = Ethernet::ETH_ARP;
      inet.nic().create_link_downstream()(std::move(pckt));
    }

    CHECK(1,"Transmission didn't panic");
    INFO("Test 1", "Done. Send some UDP-data to %s:%i to continue test",
         inet.ip_addr().str().c_str(), port);
  });

  INFO("TRANSMIT", "Ready");
}
