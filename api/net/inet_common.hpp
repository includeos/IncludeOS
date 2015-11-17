/** Common utilities for internetworking */
#ifndef INET_COMMON_HPP
#define INET_COMMON_HPP

#include <delegate>

namespace net {
  
  /** Packet must be forward declared to avoid circular dependency.
      (i.e. IP uses packet, and packet uses IP headers) */
  class Packet;
  
  const int MTUSIZE = 1500;
  const int INITIAL_BUFCOUNT = 512;
  
  /** Bundle the two - they're always together */
  using buffer = uint8_t*;
  
  using Packet_ptr = std::shared_ptr<Packet>;

  /** Downstream / upstream delegates */
  using downstream = delegate<int(Packet_ptr)>;
  using upstream = downstream;

  /** Compute the internet checksum for the buffer / buffer part provided */
  //uint16_t checksum(uint16_t* buf, uint32_t len); 
  uint16_t checksum(void* data, size_t len); 
  
}

#endif
