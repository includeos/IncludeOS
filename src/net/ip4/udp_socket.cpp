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

#include <net/ip4/udp_socket.hpp>
#include <memory>

namespace net
{
  Socket<UDP>::Socket(Inet<LinkLayer,IP4>& _stack, port port)
    : stack(_stack), l_port(port) {}
  
  int Socket<UDP>::internal_read(std::shared_ptr<PacketUDP> udp)
  {
    return on_read(*this, udp->src(), udp->src_port(), udp->data(), udp->data_length());
  }
  
  void Socket<UDP>::packet_init(std::shared_ptr<PacketUDP> p, 
      addr srcIP, addr destIP, port port, uint16_t length)
  {
    p->init();
    p->header().sport = htons(this->l_port);
    p->header().dport = htons(port);
    p->set_src(srcIP);
    p->set_dst(destIP);
    p->set_length(length);
    
    assert(p->data_length() == length);
  }
  
  int Socket<UDP>::internal_write(addr srcIP, addr destIP,
      port port, const uint8_t* buffer, int length)
  {
    // the maximum we can write per packet:
    const int WRITE_MAX = stack.MTU() - PacketUDP::HEADERS_SIZE;
    // the bytes remaining to be written
    int rem = length;
    
    while (rem >= WRITE_MAX)
    {
      // create some packet p (and convert it to PacketUDP)
      auto p = stack.createPacket(stack.MTU());
      // fill buffer (at payload position)
      memcpy(p->buffer() + PacketUDP::HEADERS_SIZE, buffer, WRITE_MAX);
      
      // initialize packet with several infos
      auto p2 = std::static_pointer_cast<PacketUDP>(p);
      packet_init(p2, srcIP, destIP, port, WRITE_MAX);
      // ship the packet
      stack.udp().transmit(p2);
      
      // next buffer part
      buffer += WRITE_MAX;  rem -= WRITE_MAX;
    }
    if (rem)
    {
      // copy remainder
      size_t size = PacketUDP::HEADERS_SIZE + rem;
      
      // create some packet p
      auto p = stack.createPacket(size);
      memcpy(p->buffer() + PacketUDP::HEADERS_SIZE, buffer, rem);
      
      // initialize packet with several infos
      auto p2 = std::static_pointer_cast<PacketUDP>(p);
      packet_init(p2, srcIP, destIP, port, rem);
      // ship the packet
      stack.udp().transmit(p2);
    }
    return length;
  } // internal_write()
  
  int Socket<UDP>::sendto(addr destIP, port port, 
                        const void* buffer, int len)
  {
    return internal_write(local_addr(), destIP, port, 
                          (const uint8_t*) buffer, len);
  }
  int Socket<UDP>::bcast(addr srcIP, port port, 
                       const void* buffer, int len)
  {
    return internal_write(srcIP, IP4::INADDR_BCAST, port, 
                          (const uint8_t*) buffer, len);
  }
  
}
