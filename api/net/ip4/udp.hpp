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

#ifndef NET_IP4_UDP_HPP
#define NET_IP4_UDP_HPP

#include <deque>
#include <map>
#include "../inet.hpp"
#include "ip4.hpp"
#include <cstring>
#include <net/packet.hpp>

namespace net {

  class PacketUDP;
  class UDPSocket;

  /** Basic UDP support. @todo Implement UDP sockets.  */
  class UDP {
  public:
    using addr_t = IP4::addr;
    using port_t = uint16_t;

    using Packet_ptr = std::unique_ptr<PacketUDP, std::default_delete<net::Packet>>;
    using Stack  = IP4::Stack;

    typedef delegate<void()> sendto_handler;

    // write buffer for sendq
    struct WriteBuffer
    {
      WriteBuffer(
                  const uint8_t* data, size_t length, sendto_handler cb,
                  UDP& udp, addr_t LA, port_t LP, addr_t DA, port_t DP);

      int remaining() const {
        return len - offset;
      }
      bool done() const {
        return offset == len;
      }

      size_t packets_needed() const;
      void write();

      // buffer, total length and current write offset
      std::shared_ptr<uint8_t> buf;
      size_t len;
      size_t offset;
      // the callback for when this buffer is written
      sendto_handler callback;
      // the UDP stack
      UDP& udp;

      // port and addr this was being sent from
      addr_t l_addr;
      port_t l_port;
      // destination address and port
      port_t d_port;
      addr_t d_addr;
    };

    /** UDP header */
    struct udp_header {
      port_t   sport;
      port_t   dport;
      uint16_t length;
      uint16_t checksum;
    };

    /** Full UDP Header with all sub-headers */
    struct full_header {
      IP4::full_header full_hdr;
      udp_header       udp_hdr;
    }__attribute__((packed));

    ////////////////////////////////////////////

    addr_t local_ip() const
    { return stack_.ip_addr(); }

    /** Input from network layer */
    void bottom(net::Packet_ptr);

    /** Delegate output to network layer */
    void set_network_out(downstream del)
    { network_layer_out_ = del; }

    /** Send UDP datagram from source ip/port to destination ip/port.

        @param sip   Local IP-address
        @param sport Local port
        @param dip   Remote IP-address
        @param dport Remote port   */
    void transmit(UDP::Packet_ptr udp);

    //! @param port local port
    UDPSocket& bind(port_t port);

    //! returns a new UDP socket bound to a random port
    UDPSocket& bind();

    bool is_bound(port_t port);

    /** Close a port **/
    void close(port_t port);

    //! construct this UDP module with @inet
    UDP(Stack& inet);

    Stack& stack()
    {
      return stack_;
    }

    // send as much as possible from sendq
    void flush();

    // create and transmit @num packets from sendq
    void process_sendq(size_t num);

    inline constexpr uint16_t max_datagram_size() noexcept {
      return stack().ip_obj().MDDS() - sizeof(udp_header);
    }

    class Port_in_use_exception : public std::exception {
    public:
      Port_in_use_exception(UDP::port_t p)
        : port_(p) {}
      virtual const char* what() const noexcept {
        return "UDP port allready in use";
      }

      UDP::port_t port(){
        return port_;
      }

    private:
      UDP::port_t port_;
    };

  private:

    downstream  network_layer_out_;
    Stack&      stack_;
    std::map<port_t, UDPSocket> ports_;
    port_t      current_port_ {1024};

    // the async send queue
    std::deque<WriteBuffer> sendq;
    friend class net::UDPSocket;
  }; //< class UDP

} //< namespace net

#include "packet_udp.hpp"
#include "udp_socket.hpp"

#endif
