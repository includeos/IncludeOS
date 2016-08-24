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

#ifndef NET_PACKET_HPP
#define NET_PACKET_HPP

#include "buffer_store.hpp"
#include "ip4/ip4.hpp"
#include <cassert>

namespace net {
  void default_packet_deleter(void* p);

  class Packet : public std::enable_shared_from_this<Packet>
  {
  public:
    using deleter_t = delegate<void(void*)>;

    /**
     *  Construct, using existing buffer.
     *
     *  @param capacity: Size of the buffer
     *  @param len: Length of data in the buffer
     *
     *  @WARNING: There are two adjacent parameters of the same type, violating CG I.24.
     */
    Packet(
        uint16_t cap, 
        uint16_t len, 
        deleter_t del = default_packet_deleter) noexcept
    : capacity_ (cap),
      size_     (len),
      deleter_  {del}
    {}
    ~Packet() {
      deleter_(this);
    }

    /** Get the buffer */
    BufferStore::buffer_t buffer() const noexcept
    { return (BufferStore::buffer_t) buf_; }

    /** Get the network packet length - i.e. the number of populated bytes  */
    inline uint16_t size() const noexcept
    { return size_; }

    /** Get the size of the buffer. This is >= len(), usually MTU-size */
    inline uint16_t capacity() const noexcept
    { return capacity_; }

    void set_size(uint16_t new_size) noexcept {
      assert((size_ = new_size) <= capacity_);
    }

    /** next-hop ipv4 address for IP routing */
    void next_hop(IP4::addr ip) noexcept {
      next_hop4_ = ip;
    }
    IP4::addr next_hop() const noexcept {
      return next_hop4_;
    }

    /* Add a packet to this packet chain.  */
    void chain(Packet_ptr p) noexcept {
      if (!chain_) {
        chain_ = p;
        last_ = p;
      } else {
        last_->chain(p);
        last_ = p->last_in_chain() ? p->last_in_chain() : p;
        assert(last_);
      }
    }

    /* Get the last packet in the chain */
    Packet_ptr last_in_chain() noexcept
    { return last_; }

    /* Get the tail, i.e. chain minus the first element */
    Packet_ptr tail() noexcept
    { return chain_; }

    /* Get the tail, and detach it from the head (for FIFO) */
    Packet_ptr detach_tail() noexcept
    {
      auto tail = chain_;
      chain_ = 0;
      return tail;
    }


    /**
     *  For a UDPv6 packet, the payload location is the start of
     *  the UDPv6 header, and so on
     */
    inline void set_payload(BufferStore::buffer_t location) noexcept
    { payload_ = location; }

    /** Get the payload of the packet */
    inline BufferStore::buffer_t payload() const noexcept
    { return payload_; }

    /**
     *  Upcast back to normal packet
     *
     *  Unfortunately, we can't upcast with std::static_pointer_cast
     *  however, all classes derived from Packet should be good to use
     */
    static Packet_ptr packet(Packet_ptr pckt) noexcept
    { return *static_cast<Packet_ptr*>(&pckt); }

    // custom deleter for Packet used by network stack
    void set_deleter(deleter_t cb) {
      this->deleter_ = cb;
    }

    // override delete to do nothing
    static void operator delete (void*) {}

  private:
    /** Let's chain packets */
    Packet_ptr chain_ {0};
    Packet_ptr last_ {0};

    /** Default constructor Deleted. See Packet(Packet&). */
    Packet() = delete;

    /**
     *  Delete copy and move because we want Packets and buffers to be 1 to 1
     *
     *  (Well, we really deleted this to avoid accidental copying)
     *
     *  The idea is to use Packet_ptr (i.e. shared_ptr<Packet>) for passing packets.
     *
     *  @todo Add an explicit way to copy packets.
     */
    Packet(Packet&) = delete;
    Packet(Packet&&) = delete;


    /** Delete copy and move assignment operators. See Packet(Packet&). */
    Packet& operator=(Packet) = delete;
    Packet operator=(Packet&&) = delete;

    uint16_t              capacity_;
    uint16_t              size_;
    IP4::addr             next_hop4_;
    deleter_t             deleter_;
    BufferStore::buffer_t payload_ {nullptr};
    BufferStore::buffer_t buf_[0];
  }; //< class Packet

  inline void default_packet_deleter(void* p) {
    delete (Packet*) p;
  }

} //< namespace net

#endif
