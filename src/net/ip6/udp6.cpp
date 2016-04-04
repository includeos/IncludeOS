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

//#define DEBUG
#include <net/ip6/udp6.hpp>

#include <alloca.h>
#include <stdio.h>

namespace net
{
  int UDPv6::bottom(Packet_ptr pckt)
  {
    debug(">>> IPv6 -> UDPv6 bottom\n");
    auto P6 = view_packet_as<PacketUDP6>(pckt);
    
    debug(">>> src port: %u \t dst port: %u\n", P6->src_port(), P6->dst_port());
    debug(">>> length: %d   \t chksum: 0x%x\n", P6->length(), P6->checksum());
    
    port_t port = P6->dst_port();
    
    // check for listeners on dst port
    if (listeners.find(port) != listeners.end())
      {
        // make the call to the listener on that port
        return listeners[port](P6);
      }
    // was not forwarded, so just return -1
    debug("... dumping packet, no listeners\n");
    return -1;
  }
  
  int UDPv6::transmit(std::shared_ptr<PacketUDP6>& pckt)
  {
    // NOTE: *** OBJECT CREATED ON STACK *** -->
    auto original = view_packet_as<PacketIP6>(pckt);
    // NOTE: *** OBJECT CREATED ON STACK *** <--
    return ip6_out(original);
  }
  
  uint16_t PacketUDP6::gen_checksum()
  {
    IP6::full_header& full = *(IP6::full_header*) this->buffer();
    IP6::header& hdr = full.ip6_hdr;
    
    // UDPv6 message + pseudo header
    uint16_t datalen = this->length() + sizeof(UDPv6::pseudo_header);
    
    // allocate it on stack
    char* data = (char*) alloca(datalen + 16);
    // unfortunately we also need to guarantee SSE aligned
    data = (char*) ((intptr_t) (data+16) & ~15); // P2ROUNDUP((intptr_t) data, 16);
    // verify that its SSE aligned
    assert(((intptr_t) data & 15) == 0);
    
    // ICMP checksum is done with a pseudo header
    // consisting of src addr, dst addr, message length (32bits)
    // 3 zeroes (8bits each) and id of the next header
    UDPv6::pseudo_header& phdr = *(UDPv6::pseudo_header*) data;
    phdr.src  = hdr.src;
    phdr.dst  = hdr.dst;
    phdr.zero = 0;
    phdr.protocol = IP6::PROTO_UDP;
    phdr.length   = htons(this->length());
    
    // reset old checksum
    header().chksum = 0;
    
    // normally we would start at &icmp_echo::type, but
    // it is after all the first element of the icmp message
    memcpy(data + sizeof(UDPv6::pseudo_header), this->payload(),
           datalen - sizeof(UDPv6::pseudo_header));
    
    // calculate csum and free data on return
    header().chksum = net::checksum(data, datalen);
    return header().chksum;
  }
  
  std::shared_ptr<PacketUDP6> UDPv6::create(
                                            Ethernet::addr ether_dest, const IP6::addr& ip6_dest, UDPv6::port_t port)
  {
    auto packet = IP6::create(IP6::PROTO_UDP, ether_dest, ip6_dest);
    auto udp_packet = view_packet_as<PacketUDP6> (packet);
    
    // set UDPv6 parameters
    udp_packet->set_src_port(666); /// FIXME: use free local port
    udp_packet->set_dst_port(port);
    udp_packet->header().chksum = 0;
    
    // set default source IP to this interface
    udp_packet->set_src(this->localIP);
    
    // make the packet empty
    udp_packet->set_length(0);
    // now, free to use :)
    return udp_packet;
  }
}
