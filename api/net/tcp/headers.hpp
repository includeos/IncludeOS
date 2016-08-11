// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#pragma once
#ifndef NET_TCP_HEADERS_HPP
#define NET_TCP_HEADERS_HPP

#include "common.hpp"
#include <net/ip4/addr.hpp>

namespace net {
namespace tcp {

/*
  Flags (Control bits) in the TCP Header.
*/
enum Flag {
  NS        = (1 << 8),     // Nounce (Experimental: see RFC 3540)
  CWR       = (1 << 7),     // Congestion Window Reduced
  ECE       = (1 << 6),     // ECN-Echo
  URG       = (1 << 5),     // Urgent
  ACK       = (1 << 4),     // Acknowledgement
  PSH       = (1 << 3),     // Push
  RST       = (1 << 2),     // Reset
  SYN       = (1 << 1),     // Syn(chronize)
  FIN       = 1,            // Fin(ish)
};

/*
  Representation of the TCP Header.

  RFC 793, (p.15):
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          Source Port          |       Destination Port        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                        Sequence Number                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                    Acknowledgment Number                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |  Data |           |U|A|P|R|S|F|                               |
  | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
  |       |           |G|K|H|T|N|N|                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |           Checksum            |         Urgent Pointer        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                    Options                    |    Padding    |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                             data                              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
struct Header {
  port_t source_port;                    // Source port
  port_t destination_port;               // Destination port
  seq_t seq_nr;                          // Sequence number
  seq_t ack_nr;                          // Acknowledge number
  union {
    uint16_t whole;                      // Reference to offset_reserved & flags together.
    struct {
      uint8_t offset_reserved;           // Offset (4 bits) + Reserved (3 bits) + NS (1 bit)
      uint8_t flags;                     // All Flags (Control bits) except NS (9 bits - 1 bit)
    };
  } offset_flags;                        // Data offset + Reserved + Flags (16 bits)
  uint16_t window_size;                  // Window size
  uint16_t checksum;                     // Checksum
  uint16_t urgent;                       // Urgent pointer offset
  uint8_t options[0];                    // Options
}__attribute__((packed)); // << struct TCP::Header


/*
  TCP Pseudo header, for checksum calculation
*/
struct Pseudo_header {
  ip4::Addr saddr;
  ip4::Addr daddr;
  uint8_t zero;
  uint8_t proto;
  uint16_t tcp_length;
}__attribute__((packed));

/*
  TCP Checksum-header (TCP-header + pseudo-header)
*/
struct Checksum_header {
  Pseudo_header pseudo;
  Header tcp;
}__attribute__((packed));

} // < namespace tcp
} // < namespace net

#endif // < NET_TCP_HEADERS_HPP
