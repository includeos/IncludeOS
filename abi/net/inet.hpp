/** Common utilities for internetworking */
#ifndef INET_HPP
#define INET_HPP

#include <delegate>
#include <net/class_packet.hpp>

namespace net {

  /** Upstream delegate */
  typedef delegate<int(std::shared_ptr<Packet>)> upstream;

  /** Compute the internet checksum for the buffer / buffer part provided */
  uint16_t checksum(uint16_t* buf, uint32_t len); 
  
}

#endif
