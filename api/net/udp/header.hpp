
#pragma once

#include <cstdint>

namespace net::udp {

using port_t = uint16_t;

/** UDP header */
struct Header {
  port_t   sport;
  port_t   dport;
  uint16_t length;
  uint16_t checksum;
};

}
