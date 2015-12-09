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

/** Common utilities for internetworking */
#ifndef INET_COMMON_HPP
#define INET_COMMON_HPP

#include <delegate>

namespace net {

  class Packet;
  class Ethernet;

  using LinkLayer = Ethernet;
  
  /** Packet must be forward declared to avoid circular dependency.
      (i.e. IP uses packet, and packet uses IP headers) */  
  const int MTUSIZE = 1500;
  const int INITIAL_BUFCOUNT = 512;
  
  /** Bundle the two - they're always together */
  using buffer = uint8_t*;
  
  using Packet_ptr = std::shared_ptr<Packet>;

  /** Downstream / upstream delegates */
  using downstream = delegate<int(Packet_ptr)>;
  using upstream = downstream;

  
  /** Packet filter delegate */
  using Packet_filter = delegate<Packet_ptr(Packet_ptr)>;
  
  /** Compute the internet checksum for the buffer / buffer part provided */
  //uint16_t checksum(uint16_t* buf, uint32_t len); 
  uint16_t checksum(void* data, size_t len); 
  
}

#endif
