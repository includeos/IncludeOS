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

//#define UDP_DEBUG 1
#ifdef UDP_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif

#include <net/udp/udp.hpp>
#include <net/udp/packet4_view.hpp>
#include <net/udp/packet6_view.hpp>
#include <common>
#include <net/inet>
#include <net/util.hpp>
#include <memory>
#include <net/ip4/icmp4.hpp>

namespace net {

  UDP::UDP(Stack& inet)
    : stack_(inet),
      ports_(inet.udp_ports())
  {
    inet.on_transmit_queue_available({this, &UDP::process_sendq});
  }

  void UDP::receive4(net::Packet_ptr ptr)
  {
    auto ip4 = static_unique_ptr_cast<PacketIP4>(std::move(ptr));
    auto pkt = std::make_unique<udp::Packet4_view>(std::move(ip4));

    const auto dst_ip = pkt->ip4_dst();
    const bool is_bcast = (dst_ip == IP4::ADDR_BCAST
      or dst_ip == stack_.broadcast_addr());

    receive(std::move(pkt), is_bcast);
  }

  void UDP::receive6(net::Packet_ptr ptr)
  {
    auto ip6 = static_unique_ptr_cast<PacketIP6>(std::move(ptr));
    auto pkt = std::make_unique<udp::Packet6_view>(std::move(ip6));
    // Bounds check **BEFORE** checksumming
    if (pkt->validate_length() == false) {
      // TODO: call drop()
      return;
    }
    // Validate checksum
    // TODO: Maybe wasteful to do checksum calc before other checks
    if (auto csum = pkt->compute_udp_checksum(); UNLIKELY(csum != 0)) {
      PRINT("<UDP::receive> UDP Packet Checksum %#x != %#x\n", csum, 0x0);
      return;
    }

    const bool is_bcast = false; // TODO: multicast?

    receive(std::move(pkt), is_bcast);
  }

  void UDP::receive(udp::Packet_view_ptr udp_packet, const bool is_bcast)
  {
    if (udp_packet->validate_length() == false) {
      PRINT("<%s> UDP: Invalid packet received (too short). Drop!\n",
              stack_.ifname().c_str());
      return;
    }

    PRINT("<%s> UDP", stack_.ifname().c_str());

    PRINT("\t Source port: %u, Dest. Port: %u Length: %u\n",
          udp_packet->src_port(), udp_packet->dst_port(), udp_packet->udp_length());

    const auto dest = udp_packet->destination();
    auto it = find(dest);
    if (it != sockets_.end()) {
      PRINT("<%s> UDP found listener on %s\n",
              stack_.ifname().c_str(), udp_packet->destination().to_string().c_str());
      it->second.internal_read(*udp_packet);
      return;
    }

    if(is_bcast) {
      auto dport = dest.port();
      PRINT("<%s> UDP received broadcast on port %d\n", stack_.ifname().c_str(), dport);

      for(auto it = sockets_.begin(); it != sockets_.end();)
      {
        auto current = it++; // internal_read() may result in close,
                             // this is to avoid iterator invalidation
        if(current->first.port() == dport)
        {
          PRINT("<%s> UDP found broadcast receiver: %s\n",
              stack_.ifname().c_str(), current->first.to_string().c_str());
          current->second.internal_read(*udp_packet);
        }
      }
      return;
    }

    PRINT("<%s> UDP: nobody listening on %u. Drop!\n",
            stack_.ifname().c_str(), udp_packet->dst_port());

    // Sending ICMP error message of type Destination Unreachable and code PORT
    // But only if the destination IP address is not broadcast or multicast
    send_dest_unreachable(std::move(udp_packet));
  }

  void UDP::send_dest_unreachable(udp::Packet_view_ptr udp)
  {
    if(udp->ipv() == Protocol::IPv4)
    {
      auto ip4 = static_unique_ptr_cast<PacketIP4>(udp->release());
      stack_.icmp().destination_unreachable(std::move(ip4), icmp4::code::Dest_unreachable::PORT);
    }
  }

  void UDP::error_report(const Error& err, Socket dest) {
    // If err is an ICMP error message:
    // Report to application layer that got an ICMP error message of type and code (reason and subreason)

    // Find callback with this destination (address and port), and call it with the incoming err
    // If the error f.ex. is an ICMP_error of type Destination Unreachable and code Fragmentation Needed,
    // the err object contains the Path MTU so the user can get a hold of it in the application layer
    // and choose to retransmit or not (the packet has been dropped by a node somewhere along the
    // path to the receiver because the Don't Fragment bit was set on the packet and the packet was
    // larger than the node's MTU value)
    auto it = error_callbacks_.find(dest);

    if (it != error_callbacks_.end()) {
      it->second.callback(err);
      error_callbacks_.erase(it);
    }
  }

  udp::Socket& UDP::bind(const net::Socket& socket)
  {
    const auto addr = socket.address();
    const auto port = socket.port();

    if(UNLIKELY( port == 0))
      return bind(addr);

    if(UNLIKELY( not stack_.is_valid_source(addr) ))
      throw UDP_error{"Cannot bind to address: " + addr.to_string()};

    auto& port_util = ports_[addr];

    if(UNLIKELY( port_util.is_bound(port) ))
      throw Port_in_use_exception{port};

    debug("<%s> UDP bind to %s\n", stack_.ifname().c_str(), socket.to_string().c_str());

    auto it = sockets_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(socket),
      std::forward_as_tuple(*this, socket));

    Ensures(it.second);

    port_util.bind(port);

    return it.first->second;
  }

  udp::Socket& UDP::bind(const addr_t& addr)
  {
    if(UNLIKELY( not stack_.is_valid_source(addr) ))
      throw UDP_error{"Cannot bind to address: " + addr.to_string()};

    auto& port_util = ports_[addr];
    const auto port = port_util.get_next_ephemeral();

    Socket socket{addr, port};
    debug("UDP bind to %s\n", socket.to_string().c_str());

    auto it = sockets_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(socket),
      std::forward_as_tuple(*this, socket));

    Ensures(it.second);

    // we know the port is not bound, else the above would throw
    port_util.bind(port);

    return it.first->second;
  }

  bool UDP::is_bound(const net::Socket& socket) const
  {
    return sockets_.find(socket) != sockets_.end();
  }

  bool UDP::is_bound(const port_t port) const
  { return is_bound({stack_.ip_addr(), port}); }

  bool UDP::is_bound6(const port_t port) const
  { return is_bound({stack_.ip6_addr(), port}); }

  void UDP::close(const net::Socket& socket)
  {
    PRINT("Closed socket %s\n", socket.to_string().c_str());
    sockets_.erase(socket);
  }

  void UDP::transmit(udp::Packet_view_ptr udp)
  {
    PRINT("<UDP> Transmitting %u bytes (data=%u) from %s to %s:%i\n",
           udp->udp_length(), udp->udp_data_length(),
           udp->ip_src().to_string().c_str(),
           udp->ip_dst().to_string().c_str(), udp->dst_port());

    Expects(udp->udp_length() >= sizeof(udp::Header));

    if(udp->ipv() == Protocol::IPv6) {
      udp->set_udp_checksum(); // mandatory in IPv6
      network_layer_out6_(udp->release());
    }
    else {
      //udp->set_udp_checksum(); // optional in IPv4
      network_layer_out4_(udp->release());
    }
  }

  void UDP::flush()
  {
    size_t packets = stack_.transmit_queue_available();
    if (packets) process_sendq(packets);
  }

  void UDP::flush_expired() {
    PRINT("<UDP> Flushing expired error callbacks\n");

    for (auto it = error_callbacks_.begin(); it != error_callbacks_.end();)
    {
      if (not it->second.expired())
        it++;
      else
        it = error_callbacks_.erase(it);
    }

    if (not error_callbacks_.empty())
      flush_timer_.start(flush_interval_);
  }

  void UDP::process_sendq(size_t num)
  {
    while (!sendq.empty() && num != 0)
    {
      WriteBuffer& buffer = sendq.front();

      // If nothing is remaining, it probably means we're currently
      // processing this buffer
      if (UNLIKELY(buffer.remaining() == 0))
        return;

      // create and transmit packet from writebuffer
      buffer.write();
      num--;

      if (buffer.done()) {
        if (buffer.send_callback != nullptr)
          buffer.send_callback();

        if (buffer.error_callback != nullptr) {
          error_callbacks_.emplace(std::piecewise_construct,
                              std::forward_as_tuple(buffer.dst),
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

  UDP::WriteBuffer::WriteBuffer(UDP& stack, net::Socket source, net::Socket dest,
                                const uint8_t* data, size_t length,
                                sendto_handler cb, error_handler ecb)
    : udp(stack),
      src{std::move(source)}, dst{std::move(dest)},
      len(length), offset(0),
      send_callback(cb), error_callback(ecb)
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
    udp::Packet_view_ptr chain_head = nullptr;

    PRINT("<%s> UDP: %i bytes to write, need %i packets \n",
          udp.stack().ifname().c_str(),
          remaining(),
          remaining() / udp.max_datagram_size() + (remaining() % udp.max_datagram_size() ? 1 : 0));

    // Only call write() (above) when there's something remaining
    Expects(remaining());

    while (remaining()) {
      // the max bytes we can write in one operation
      size_t total = remaining();
      total = (total > udp.max_datagram_size()) ? udp.max_datagram_size() : total;

      // Create IP packet and convert it to PacketUDP)
      auto pkt = udp.create_packet(src, dst);
      if (!pkt) break;

      pkt->fill(buf.get() + this->offset, total);

      // Attach packet to chain
      if (!chain_head)
        chain_head = std::move(pkt);
      else
        chain_head->packet_ptr()->chain(pkt->release());

      // next position in buffer
      this->offset += total;
    }

    // Only transmit if a chain actually was produced
    if (chain_head) {
      // ship the packet
      udp.transmit(std::move(chain_head));
    }
  }

  udp::Packet_view_ptr UDP::create_packet(const net::Socket& src,
                                          const net::Socket& dst)
  {
    if(src.address().is_v6())
    {
      Expects(dst.address().is_v6());
      auto pkt = std::make_unique<udp::Packet6_view>(stack_.create_ip6_packet(Protocol::UDP));
      pkt->init(src, dst);
      return pkt;
    }
    else
    {
      Expects(dst.address().is_v4());
      auto pkt = std::make_unique<udp::Packet4_view>(stack_.create_ip_packet(Protocol::UDP));
      pkt->init(src, dst);
      return pkt;
    }
  }

  UDP::addr_t UDP::local_ip() const
  { return stack_.ip_addr(); }

  udp::Socket& UDP::bind(port_t port)
  { return bind({stack_.ip_addr(), port}); }

  udp::Socket& UDP::bind6(port_t port)
  { return bind({stack_.ip6_addr(), port}); }

  udp::Socket& UDP::bind()
  { return bind(stack_.ip_addr()); }

  udp::Socket& UDP::bind6()
  { return bind(stack_.ip6_addr()); }

  uint16_t UDP::max_datagram_size() noexcept
  { return stack().ip_obj().MDDS() - sizeof(udp::Header); }

} //< namespace net
