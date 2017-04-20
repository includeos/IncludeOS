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
#include <net/ip4/icmp4.hpp>

namespace net {

  UDP::UDP(Stack& inet)
    : network_layer_out_{[] (net::Packet_ptr) {}},
      stack_(inet),
      ports_{},
      current_port_{new_ephemeral_port()}
  {
    inet.on_transmit_queue_available({this, &UDP::process_sendq});
  }

  void UDP::receive(net::Packet_ptr pckt)
  {
    auto udp_packet = static_unique_ptr_cast<PacketUDP>(std::move(pckt));

    debug("<%s> UDP", stack_.ifname().c_str());

    debug("\t Source port: %u, Dest. Port: %u Length: %u\n",
          udp_packet->src_port(), udp_packet->dst_port(), udp_packet->length());

    auto it = ports_.find(udp_packet->dst_port());
    if (LIKELY(it != ports_.end())) {
      debug("<%s> UDP found listener on port %u\n",
              stack_.ifname().c_str(), udp_packet->dst_port());
      it->second.internal_read(std::move(udp_packet));
      return;
    }

    debug("<%s> UDP: nobody listening on %u. Drop!\n",
            stack_.ifname().c_str(), udp_packet->dst_port());

    // Sending ICMP error message of type Destination Unreachable and code PORT
    // But only if the destination IP address is not broadcast or multicast
    auto ip4_packet = static_unique_ptr_cast<PacketIP4>(std::move(udp_packet));
    if (ip4_packet->ip_dst() != stack_.broadcast_addr() and ip4_packet->ip_dst() != IP4::ADDR_BCAST and
      not ip4_packet->ip_dst().is_multicast())
    {
      stack_.icmp().destination_unreachable(std::move(ip4_packet), icmp4::code::Dest_unreachable::PORT);
    }
  }

  void UDP::error_report(Error& err, Socket dest) {
    // If err is an ICMP error message:
    // Report to application layer that got an ICMP error message of type and code (reason and subreason)

    // Find callback with this destination address and port, and call it with the incoming err
    auto it = error_callbacks_.find(Socket{dest.address(), dest.port()});

    if (it != error_callbacks_.end()) {
      it->second.callback(err);
      error_callbacks_.erase(it);
    }
  }

  UDPSocket& UDP::bind(UDP::port_t port)
  {
    debug("<%s> UDP bind to port %d\n", stack_.ifname().c_str(), port);
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
    } else {
      throw UDP::Port_in_use_exception(it->first);
    }
    return it->second;
  }

  UDPSocket& UDP::bind()
  {
    if (UNLIKELY(ports_.size() >= (port_ranges::DYNAMIC_END - port_ranges::DYNAMIC_START)))
      throw std::runtime_error("UPD Socket: All ports taken!");

    while (ports_.find(++current_port_) != ports_.end())
      // prevent automatic ports under 1024
      if (current_port_  == port_ranges::DYNAMIC_END) current_port_ = port_ranges::DYNAMIC_START;

    return bind(current_port_);
  }

  bool UDP::is_bound(UDP::port_t port)
  {
    return ports_.find(port) != ports_.end();
  }

  void UDP::close(UDP::port_t port)
  {
    debug("Closed port %u\n", port);
    if (is_bound(port))
      ports_.erase(port);
  }

  void UDP::transmit(UDP::Packet_ptr udp)
  {
    debug("<UDP> Transmitting %u bytes (data=%u) from %s to %s:%i\n",
           udp->length(), udp->data_length(),
           udp->ip_src().str().c_str(),
           udp->ip_dst().str().c_str(), udp->dst_port());

    Expects(udp->length() >= sizeof(header));
    Expects(udp->ip_protocol() == Protocol::UDP);

    network_layer_out_(std::move(udp));
  }

  void UDP::flush()
  {
    size_t packets = stack_.transmit_queue_available();
    if (packets) process_sendq(packets);
  }

  void UDP::flush_expired() {
    INFO("UDP", "Flushing expired error callbacks");

    for (auto& err : error_callbacks_) {
      if (err.second.expired())
        error_callbacks_.erase(err.first);
    }

    if (not error_callbacks_.empty())
      flush_timer_.start(flush_interval_);
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
        if (buffer.send_callback != nullptr)
          buffer.send_callback();

        if (buffer.error_callback != nullptr) {
          error_callbacks_.emplace(std::piecewise_construct,
                              std::forward_as_tuple(Socket{buffer.d_addr, buffer.d_port}),
                              std::forward_as_tuple(Error_entry{buffer.error_callback}));

          if (UNLIKELY(not flush_timer_.is_running()))
            flush_timer_.start(flush_interval_);
        }

        // remove buffer from queue
        sendq.pop_front();

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

  UDP::WriteBuffer::WriteBuffer(const uint8_t* data, size_t length, sendto_handler cb, error_handler ecb,
                                UDP& stack, addr_t LA, port_t LP, addr_t DA, port_t DP)
    : len(length), offset(0), send_callback(cb), error_callback(ecb), udp(stack),
      l_addr(LA), l_port(LP), d_port(DP), d_addr(DA)
  {
    // create a copy of the data,
    auto* copy = new uint8_t[len];
    memcpy(copy, data, length);
    // make it shared
    this->buf =
      std::shared_ptr<uint8_t> (copy, std::default_delete<uint8_t[]>());
  }

  void UDP::WriteBuffer::write()
  {
    UDP::Packet_ptr chain_head = nullptr;

    debug("<%s> UDP: %i bytes to write, need %i packets \n",
          udp.stack().ifname().c_str(),
          remaining(),
          remaining() / udp.max_datagram_size() + (remaining() % udp.max_datagram_size() ? 1 : 0));

    while (remaining()) {
      // the max bytes we can write in one operation
      size_t total = remaining();
      total = (total > udp.max_datagram_size()) ? udp.max_datagram_size() : total;

      // Create IP packet and convert it to PacketUDP)
      auto p = udp.stack().create_ip_packet(Protocol::UDP);
      if (!p) break;

      auto p2 = static_unique_ptr_cast<PacketUDP>(std::move(p));
      // Initialize UDP packet
      p2->init(l_port, d_port);
      p2->set_ip_src(l_addr);
      p2->set_ip_dst(d_addr);
      p2->set_data_length(total);

      // fill buffer (at payload position)
      memcpy(p2->data(), buf.get() + this->offset, total);

      // Attach packet to chain
      if (!chain_head)
        chain_head = std::move(p2);
      else
        chain_head->chain(std::move(p2));

      // next position in buffer
      this->offset += total;
    }

    Expects(chain_head->ip_protocol() == Protocol::UDP);
    // ship the packet
    udp.transmit(std::move(chain_head));
  }

} //< namespace net
