
#pragma once
#ifndef NET_IP4_HEADER_HPP
#define NET_IP4_HEADER_HPP

#include <net/ip4/addr.hpp>

namespace net {
namespace ip4 {

/**
 * This type contain the valid set of flags that can be set on an
 * IPv4 datagram
 */
enum class Flags : uint8_t {
  NONE = 0b000,
  MF   = 0b001,
  DF   = 0b010,
  MFDF = 0b011
}; //< enum class Flags

/**
 * This type is used to represent the standard IPv4 header
 */
struct Header {
  uint8_t  version_ihl = 0x45;  //< IP version and header length
  uint8_t  ds_ecn      = 0;     //< IP datagram differentiated services codepoint and explicit congestion notification
  uint16_t tot_len     = 0;     //< IP datagram total length
  uint16_t id          = 0;     //< IP datagram identification number
  uint16_t frag_off_flags = 0;  //< IP datagram fragment offset and control flags
  uint8_t  ttl;            //< IP datagram time to live value
  uint8_t  protocol;       //< IP datagram protocol value
  uint16_t check;          //< IP datagram checksum value
  Addr     saddr;          //< IP datagram source address
  Addr     daddr;          //< IP datagram destination address
}; //< struct Header

} //< namespace ip4
} //< namespace net

#endif //< NET_IP4_HEADER_HPP
