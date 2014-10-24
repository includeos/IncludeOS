/** Common utilities for internetworking */
#ifndef INET_HPP
#define INET_HPP

namespace net {

  /** Compute the internet checksum for the buffer / buffer part provided */
  uint16_t checksum(uint16_t* buf, uint32_t len); 
  
}

#endif
