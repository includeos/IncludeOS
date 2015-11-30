// Part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud
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
#include <net/inet>
#include <net/util.hpp>
#include <net/class_packet.hpp>
#include <net/class_udp.hpp>
#include <string>
#include <iostream>

#include <memstream>
#include "packet_store.hpp"

using namespace net;
Inet* network;
PacketStore packetStore(50, 1500);

void reflect(Inet*, Packet_ptr);

static const int SERVICE_PORT = 12345;

void Service::start()
{
  Inet::ifconfig(net::ETH0,
                 { 10,  0, 0, 2},
                 {255,255, 0, 0},
                 {0, 0});
	
	network = Inet::up();
	
  network->udp_listen(SERVICE_PORT,
  [] (net::Packet_ptr pckt)
  {
    std::cout << "*** Received data on port " << SERVICE_PORT << std::endl;
    reflect(network, pckt);
    
    return 0;
  });
}

void reflect(Inet* network, Packet_ptr packet)
{
  //auto pckt = packetStore.getPacket();
  UDP::full_header& header = *(UDP::full_header*) packet->buffer();
  
  // Populate outgoing UDP header
  header.udp_hdr.dport = header.udp_hdr.sport;
  header.udp_hdr.sport = htons(SERVICE_PORT);
  //header.udp_hdr.length = htons(sizeof(UDP::udp_header) + messageSize);
  
  // Populate outgoing IP header
  header.ip_hdr.daddr = header.ip_hdr.saddr;
  header.ip_hdr.saddr = network->ip4(ETH0);
  header.ip_hdr.protocol = IP4::IP4_UDP;
  
  // packet payload
  //streamucpy((char*) pckt->buffer() + sizeof(UDP::full_header), this->buffer, messageSize);
  //pckt->set_len(sizeof(UDP::full_header) + messageSize);
  
  std::cout << "*** Responding to " << header.ip_hdr.daddr.str() << ", "
            << "len = " << htons(header.udp_hdr.length) << std::endl;
  network->udp_send(packet);
}
