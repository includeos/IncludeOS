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

#pragma once

#include <net/tcp.hpp>
#include "packet_ip4.hpp"
#include <cassert>

namespace net
{
  
  /** A TCP Packet wrapper, with no data just methods. */
  class TCP_packet : public PacketIP4, // might work as upcast:
		     public std::enable_shared_from_this<TCP_packet>
  {
  public:
    
    inline TCP::tcp_header& header() const
    {
      return ((TCP::full_header*) buffer())->tcp_hdr;
    }
    
    static const size_t HEADERS_SIZE = sizeof(TCP::full_header);
    
    //! initializes to a default, empty TCP packet, given
    //! a valid MTU-sized buffer
    void init()
    {            
      // Erase all headers (smart? necessary? ...well, convenient)
      memset(buffer(), 0, sizeof(TCP::full_header));
      PacketIP4::init();
      
      set_protocol(IP4::IP4_TCP);
      set_win_size(TCP::default_window_size);
      
      // set TCP payload location (!?)
      payload_ = buffer() + sizeof(TCP::full_header);
    }
    
    
    inline TCP::Port src_port() const
    {
      return ntohs(header().sport);
    }

    inline TCP::Port dst_port() const
    {
      return ntohs(header().dport);
    }
        
    inline void set_dport(TCP::Port p)
    { header().dport = htons(p); }
    
    inline void set_sport(TCP::Port p)
    { header().sport = htons(p); }
    
    inline void set_seqnr(decltype(TCP::tcp_header::seq_nr) n)
    { header().seq_nr = htonl(n); }

    inline void set_acknr(decltype(TCP::tcp_header::ack_nr) n)
    { header().ack_nr = htonl(n); }
    
    inline void set_flag(TCP::Flag f)
    { header().set_flag(f); }

    inline void set_flags(uint16_t f)
    { header().set_flags(f); }
    
    inline void set_win_size(uint16_t size)
    { header().win_size = htons(size); }

    inline bool isset(TCP::Flag f)
    { return ntohs(header().offs_flags.whole) & f; }
    
    inline uint16_t data_length() const
    {      
      return size() - header().all_headers_len();
    }
    
    inline uint16_t tcp_length() const 
    {
      return data_length() + header().size();
    }
    
    inline char* data()
    {
      return (char*) (buffer() + sizeof(TCP::full_header));
    }
    
    // sets the correct length for all the protocols up to IP4
    void set_length(uint16_t newlen)
    {
      // new total packet length
      set_size( sizeof(TCP::full_header) + newlen );
    }
    
    // generates a new checksum and sets it for this TCP packet
    uint16_t gen_checksum(){ return TCP::checksum(Packet::shared_from_this()); }
    
    //! assuming the packet has been properly initialized,
    //! this will fill bytes from @buffer into this packets buffer,
    //! then return the number of bytes written. buffer is unmodified
    uint32_t fill(const std::string& buffer)
    {
      uint32_t rem = capacity();
      uint32_t total = (buffer.size() < rem) ? buffer.size() : rem;
      // copy from buffer to packet buffer
      memcpy(data() + data_length(), buffer.data(), total);
      // set new packet length
      set_length(data_length() + total);
      return total;
    }
  };
}
