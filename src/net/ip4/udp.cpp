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

#define DEBUG
#include <os>
#include <net/ip4/udp.hpp>
#include <net/util.hpp>
#include <memory>

namespace net {

  UDP::UDP(Stack& inet)
    : stack_(inet)
  {
    network_layer_out_ = [] (net::Packet_ptr) {};
    inet.on_transmit_queue_available(
      transmit_avail_delg::from<UDP, &UDP::process_sendq>(this));
  }
  
  void UDP::bottom(net::Packet_ptr pckt)
  {
    debug("<UDP handler> Got data");
    std::shared_ptr<PacketUDP> udp = 
      std::static_pointer_cast<PacketUDP> (pckt);
  
    debug("\t Source port: %i, Dest. Port: %i Length: %i\n",
          udp->src_port(), udp->dst_port(), udp->length());
  
    auto it = ports_.find(udp->dst_port());
    if (it != ports_.end())
      {
        debug("<UDP> Someone's listening to this port. Forwarding...\n");
        it->second.internal_read(udp);
      }
  
    debug("<UDP> Nobody's listening to this port. Drop!\n");
  }

  UDPSocket& UDP::bind(UDP::port_t port)
  {
    debug("<UDP> Binding to port %i\n", port);
    /// ... !!!
    auto it = ports_.find(port);
    if (it == ports_.end()) {
      // create new socket
      auto res = ports_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(port),
        std::forward_as_tuple(stack_, port));
      it = res.first;
    }
    return it->second;
  }

  UDPSocket& UDP::bind() {  

    if (ports_.size() >= 0xfc00)
      panic("UPD Socket: All ports taken!");  

    debug("UDP finding free ephemeral port\n");  
    while (ports_.find(++current_port_) != ports_.end())
      // prevent automatic ports under 1024
      if (current_port_  == 0) current_port_ = 1024;
  
    debug("UDP binding to %i port\n", current_port_);
    return bind(current_port_);
  }

  void UDP::transmit(UDP::Packet_ptr udp) {
    debug2("<UDP> Transmitting %i bytes (seg=%i) from %s to %s:%i\n",
           udp->length(), udp->ip4_segment_size(),
           udp->src().str().c_str(),
           udp->dst().str().c_str(), udp->dst_port());
  
    assert(udp->length() >= sizeof(udp_header));
    assert(udp->protocol() == IP4::IP4_UDP);
  
    auto pckt = Packet::packet(udp);
    network_layer_out_(pckt);
  }
  
  void UDP::process_sendq(size_t num)
  {
    while (!sendq.empty() && num != 0)
    {
      WriteBuffer& buffer = sendq.front();
      // create and transmit packet from writebuffer
      buffer.write();
      num--;
      
      if (buffer.done())
      {
        // call on_written callback
        buffer.callback();
        // remove buffer from queue
        sendq.pop_front();
      }
    }
  }
  
  size_t UDP::WriteBuffer::packets_needed() const
  {
    int r = remaining();
    // whole packets
    size_t P = r / udp.stack().MTU();
    // one packet for remainder
    if (r % udp.stack().MTU()) P++;
    return P;
  }
  UDP::WriteBuffer::WriteBuffer(
      const uint8_t* data, size_t length, sendto_handler cb,
      UDP& stack, addr_t LA, port_t LP, addr_t DA, port_t DP)
  : len(length), offset(0), callback(cb), udp(stack),
    l_addr(LA), l_port(LP), d_port(DP), d_addr(DA)
  {
    // create a copy of the data,
    auto* copy = new uint8_t[len];
    memcpy(copy, data, length);
    // make it shared
    this->buf = 
      std::shared_ptr<uint8_t> (copy, std::default_delete<uint8_t[]>());
  }
  
  void UDP::WriteBuffer::write()
  {
    const size_t MTU = udp.stack().MTU();
    
    // the maximum we can write per packet:
    const size_t WRITE_MAX = MTU - PacketUDP::HEADERS_SIZE;
    // the bytes remaining to be written
    size_t total = remaining();
    total = (total > WRITE_MAX) ? WRITE_MAX : total;
    
    // create some packet p (and convert it to PacketUDP)
    auto p = udp.stack().createPacket(MTU);
    // fill buffer (at payload position)
    memcpy(p->buffer() + PacketUDP::HEADERS_SIZE, 
           buf.get() + this->offset, total);
    
    // initialize packet with several infos
    auto p2 = std::static_pointer_cast<PacketUDP>(p);
    
    p2->init();
    p2->header().sport = htons(l_port);
    p2->header().dport = htons(d_port);
    p2->set_src(l_addr);
    p2->set_dst(d_addr);
    p2->set_length(total);
    
    // ship the packet
    udp.transmit(p2);
    
    // next position in buffer
    this->offset += total;
  }
  
} //< namespace net
