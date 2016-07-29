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

#ifndef CLASS_IP4_HPP
#define CLASS_IP4_HPP

#include <regex>
#include <string>
#include <iostream>

#include <net/ethernet.hpp>
#include <net/inet.hpp>

namespace net {

  // Default delegate assignments
  void ignore_ip4_up(Packet_ptr);
  void ignore_ip4_down(Packet_ptr);

  /** IP4 layer */
  class IP4 {
  public:
    /** Initialize. Sets a dummy linklayer out. */
    explicit IP4(Inet<LinkLayer, IP4>&) noexcept;

    /** Known transport layer protocols. */
    enum proto { IP4_ICMP=1, IP4_UDP=17, IP4_TCP=6 };

    /** IP4 address representation */
    struct addr {
      uint32_t whole;
      
      addr() : whole(0) {} // uninitialized
      addr(uint32_t ipaddr)
        : whole(ipaddr) {}
      addr(uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4)
        : whole(p1 | (p2 << 8) | (p3 << 16) | (p4 << 24)) {}

      /**
       * @brief Construct an IPv4 address from a {std::string}
       * object
       *
       * @note If the {std::string} object doesn't contain a valid
       * IPv4 representation then the instance will contain the
       * address -> 0.0.0.0
       *
       * @param ip_addr:
       * A {std::string} object representing an IPv4 address
       */
      addr(const std::string& ip_addr)
        : addr{}
      {
        const static std::regex ipv4_address_pattern
        {
          "^(25[0–5]|2[0–4]\\d|[01]?\\d\\d?)\\."
          "(25[0–5]|2[0–4]\\d|[01]?\\d\\d?)\\."
          "(25[0–5]|2[0–4]\\d|[01]?\\d\\d?)\\."
          "(25[0–5]|2[0–4]\\d|[01]?\\d\\d?)$"
        };

        std::smatch ipv4_parts;
      
        if (not std::regex_match(ip_addr, ipv4_parts, ipv4_address_pattern)) {
          return;
        }
        
        auto p1 = static_cast<uint8_t>(std::stoi(ipv4_parts[1]));
        auto p2 = static_cast<uint8_t>(std::stoi(ipv4_parts[2]));
        auto p3 = static_cast<uint8_t>(std::stoi(ipv4_parts[3]));
        auto p4 = static_cast<uint8_t>(std::stoi(ipv4_parts[4]));

        whole = p1 | (p2 << 8) | (p3 << 16) | (p4 << 24);
      }
      
      inline addr& operator=(addr cpy) noexcept {
        whole = cpy.whole;
        return *this;
      }

      /** Standard comparison operators */
      bool operator==(addr rhs)           const noexcept
      { return whole == rhs.whole; }

      bool operator==(const uint32_t rhs) const noexcept
      { return  whole == rhs; }

      bool operator<(const addr rhs)      const noexcept
      { return whole < rhs.whole; }

      bool operator<(const uint32_t rhs)  const noexcept
      { return  whole < rhs; }

      bool operator>(const addr rhs)      const noexcept
      { return whole > rhs.whole; }

      bool operator>(const uint32_t rhs)  const noexcept
      { return  whole > rhs; }

      bool operator!=(const addr rhs)     const noexcept
      { return whole != rhs.whole; }

      bool operator!=(const uint32_t rhs) const noexcept
      { return  whole != rhs; }

      addr operator & (addr rhs) const noexcept
      { return addr(whole & rhs.whole); }
      
      /** x.x.x.x string representation */
      std::string str() const {
        char ip_addr[16];
        sprintf(ip_addr, "%1i.%1i.%1i.%1i",
                (whole >>  0) & 0xFF,
                (whole >>  8) & 0xFF, 
                (whole >> 16) & 0xFF, 
                (whole >> 24) & 0xFF);
        return ip_addr;
      }
    } __attribute__((packed)); //< IP4::addr

    static const addr INADDR_ANY;
    static const addr INADDR_BCAST;

    /** IP4 header representation */
    struct ip_header {
      uint8_t  version_ihl;
      uint8_t  tos;
      uint16_t tot_len;
      uint16_t id;
      uint16_t frag_off_flags;
      uint8_t  ttl;
      uint8_t  protocol;
      uint16_t check;
      addr     saddr;
      addr     daddr;
    };

    /**
     *  The full header including IP
     *
     *  @Note: This might be removed if we decide to isolate layers more
     */
    struct full_header {
      uint8_t   link_hdr[sizeof(typename LinkLayer::header)];
      ip_header ip_hdr;
    };

    /*
      Maximum Datagram Data Size
    */
    inline constexpr uint16_t MDDS() const
    { return stack_.MTU() - sizeof(ip_header); }

    /** Upstream: Input from link layer */
    void bottom(Packet_ptr);

    /** Upstream: Outputs to transport layer */
    inline void set_icmp_handler(upstream s)
    { icmp_handler_ = s; }

    inline void set_udp_handler(upstream s)
    { udp_handler_ = s; }

    inline void set_tcp_handler(upstream s)
    { tcp_handler_ = s; }

    /** Downstream: Delegate linklayer out */
    void set_linklayer_out(downstream s)
    { linklayer_out_ = s; };

    /**
     *  Downstream: Receive data from above and transmit
     *
     *  @note: The following *must be set* in the packet:
     *
     *   * Destination IP
     *   * Protocol
     *
     *  Source IP *can* be set - if it's not, IP4 will set it
     */
    void transmit(Packet_ptr);

    /** Compute the IP4 header checksum */
    uint16_t checksum(ip_header*);

    /**
     * \brief
     *
     * Returns the IPv4 address associated with this interface
     **/
    const addr local_ip() const {
      return stack_.ip_addr();
    }

  private:
    Inet<LinkLayer,IP4>& stack_;

    /** Downstream: Linklayer output delegate */
    downstream linklayer_out_ {ignore_ip4_down};

    /** Upstream delegates */
    upstream icmp_handler_ {ignore_ip4_up};
    upstream udp_handler_  {ignore_ip4_up};
    upstream tcp_handler_  {ignore_ip4_up};
  }; //< class IP4
} //< namespace net

#endif
