
#pragma once
#ifndef NET_IP6_HEADER_HPP
#define NET_IP6_HEADER_HPP

#include <net/ip6/addr.hpp>
#define  IP6_HEADER_LEN 40
#define  IP6_ADDR_BYTES 16

namespace net::ip6 {

/**
 * This type is used to represent the standard IPv6 header
 */
struct Header {
  union {
    uint32_t ver_tc_fl = 0x0060;
    uint32_t version : 4,
             traffic_class : 8,
             flow_label : 20;
  };
  uint16_t payload_length = 0;
  uint8_t  next_header = 0;
  uint8_t  hop_limit   = 0;
  Addr     saddr;
  Addr     daddr;
} __attribute__((packed)); //< struct Header

static_assert(sizeof(Header) == 40, "IPv6 Header is of constant size (40 bytes)");

} //< namespace net::ip6

#endif //< NET_IP6_HEADER_HPP
