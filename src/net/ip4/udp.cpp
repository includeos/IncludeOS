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
#include <common>
#include <net/ip4/udp.hpp>
#include <net/util.hpp>
#include <memory>

namespace net {

UDP::UDP(Stack& inet)
  : stack_(inet)
{
  network_layer_out_ = [] (net::Packet_ptr) {};
  inet.on_transmit_queue_available({this, &UDP::process_sendq});
}

void UDP::bottom(net::Packet_ptr pckt)
{
  auto udp = static_unique_ptr_cast<PacketUDP>(std::move(pckt));

  debug("\t Source port: %i, Dest. Port: %i Length: %i\n",
        udp->src_port(), udp->dst_port(), udp->length());

  auto it = ports_.find(udp->dst_port());
  if (LIKELY(it != ports_.end())) {
    it->second.internal_read(std::move(udp));
    return;
  }

  debug("<UDP> Nobody's listening to this port. Drop!\n");
}

UDPSocket& UDP::bind(UDP::port_t port)
{
  /// bind(0) == bind()
  if (port == 0) return bind();
  /// ... !!!
  auto it = ports_.find(port);
  if (LIKELY(it == ports_.end())) {
    // create new socket
    auto res = ports_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(port),
        std::forward_as_tuple(*this, port));
    it = res.first;
  }else {
    throw UDP::Port_in_use_exception(it->first);
  }
  return it->second;
}

UDPSocket& UDP::bind()
{
  if (UNLIKELY(ports_.size() >= 0xfc00))
      throw std::runtime_error("UPD Socket: All ports taken!");

  while (ports_.find(++current_port_) != ports_.end())
    // prevent automatic ports under 1024
    if (current_port_  == 0) current_port_ = 1024;

  debug("UDP bind to port %d\n", current_port_);
  return bind(current_port_);
}

bool UDP::is_bound(UDP::port_t port)
{
  return ports_.find(port) != ports_.end();
}

void UDP::close(UDP::port_t port)
{
  if (is_bound(port))
    ports_.erase(port);
}

void UDP::transmit(UDP::Packet_ptr udp)
{
  debug2("<UDP> Transmitting %i bytes (seg=%i) from %s to %s:%i\n",
         udp->length(), udp->ip4_segment_size(),
         udp->src().str().c_str(),
         udp->dst().str().c_str(), udp->dst_port());

  assert(udp->length() >= sizeof(udp_header));
  assert(udp->protocol() == IP4::IP4_UDP);

  network_layer_out_(std::move(udp));
}

void UDP::flush()
{
  size_t packets = stack_.transmit_queue_available();
  if (packets) process_sendq(packets);
}

void UDP::process_sendq(size_t num)
{
  while (!sendq.empty() && num != 0)
  {
    WriteBuffer& buffer = sendq.front();

    // create and transmit packet from writebuffer
    buffer.write();
    num--;

    if (buffer.done()) {
      auto copy = buffer.callback;
      // remove buffer from queue
      sendq.pop_front();
      // call on_written callback
      copy();
      // reduce @num, just in case packets were sent in
      // another stack frame
      size_t avail = stack_.transmit_queue_available();
      num = (num > avail) ? avail : num;
    }
  }
}

size_t UDP::WriteBuffer::packets_needed() const
{
  int r = remaining();
  // whole packets
  size_t P = r / udp.max_datagram_size();
  // one packet for remainder
  if (r % udp.max_datagram_size()) P++;
  return P;
}
UDP::WriteBuffer::WriteBuffer(const uint8_t* data, size_t length, sendto_handler cb,
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

  // the bytes remaining to be written
  UDP::Packet_ptr chain_head{};

  debug("<UDP> %i bytes to write, need %i packets \n",
         remaining(), remaining() / udp.max_datagram_size() + (remaining() % udp.max_datagram_size() ? 1 : 0));

  while (remaining())
  {
    size_t total = remaining();
    total = (total > udp.max_datagram_size()) ? udp.max_datagram_size() : total;

    // create some packet p (and convert it to PacketUDP)
    auto p = udp.stack().create_packet(0);
    // fill buffer (at payload position)
    memcpy(p->buffer() + PacketUDP::HEADERS_SIZE,
           buf.get() + this->offset, total);

    // initialize packet with several infos
    auto p2 = static_unique_ptr_cast<PacketUDP>(std::move(p));

    p2->init();
    p2->header().sport = htons(l_port);
    p2->header().dport = htons(d_port);
    p2->set_src(l_addr);
    p2->set_dst(d_addr);
    p2->set_length(total);

    // Attach packet to chain
    if (!chain_head)
      chain_head = std::move(p2);
    else
      chain_head->chain(std::move(p2));

    // next position in buffer
    this->offset += total;
  }

  // ship the packet
  udp.transmit(std::move(chain_head));
}

} //< namespace net
