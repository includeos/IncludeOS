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
#include "ip4/addr.hpp"
#include <gsl/gsl_assert>
#include <delegate>
#include <cassert>

namespace net
{
  class Packet;
  using Packet_ptr = std::unique_ptr<Packet>;

  class Packet
  {
  public:
    using Byte = uint8_t;
    using Byte_ptr = Byte*;

    /**
     *  Construct, using existing buffer.
     *
     *  @param offs_layer_begin : Pointer to where the current protocol layer starts
     *  @param offs_data_end :    Pointer to data end, e.g. next to last write
     *  @param offs_buffer_end :  Pointer to byte after last in buffer
     *  @param bufstore :         Pointer to buffer store owning this buffer
     *
     *  @WARNING: There are adjacent parameters of the same type, violating CG I.24.
     *  Note that the order is strict from left to right: begin, data
     */
    Packet(int offs_layer_begin,
           int offs_data_end,
           int offs_buffer_end,
           BufferStore* bufstore) noexcept
      : layer_begin_(buf() + offs_layer_begin),
        data_end_(layer_begin() + offs_data_end),
        buffer_end_(buf() + offs_buffer_end),
        bufstore_(bufstore)
    {
      Expects(offs_layer_begin >= 0 and
              buf() + offs_layer_begin <= buffer_end() and
              data_end() <= buffer_end());
    }

    ~Packet()
    {
      if (bufstore_)
          bufstore_->release(this);
      else
          delete[] (Byte_ptr) this;
    }

    /** Get the buffer */
    Byte_ptr buf() noexcept
    { return &buf_[0]; }

    const Byte* buf() const noexcept
    { return &buf_[0]; }

    /** Get the start of the layer currently being accessed */
    Byte_ptr layer_begin() const noexcept
    { return layer_begin_; }

    /** Get the write position */
    Byte_ptr data_end() const noexcept
    { return data_end_; }

    /** Get the end of the buffer, e.g. pointer to byte after last */
    const Byte* buffer_end() const noexcept
    { return buffer_end_; }

    /** Get the number of populated bytes relative to current layer start */
    int size() const noexcept
    { return data_end_ - layer_begin_; }

    /** Get the total size of data portion, >= size() and MTU-like */
    int capacity() const noexcept
    { return buffer_end_ - layer_begin_; }

    int bufsize() const noexcept
    { return buffer_end() - buf(); }

    /** Increment / decrement layer_begin, resets data_end */
    void increment_layer_begin(int i)
    {
      set_layer_begin(layer_begin() + i);
    }

    /** Set data end / write-position relative to layer_begin */
    void set_data_end(int offset)
    {
      Expects(offset >= 0 and layer_begin() + offset <= buffer_end_);
      data_end_ = layer_begin() + offset;
    }

    void increment_data_end(int i)
    {
      Expects(i > 0 && data_end_ + i <= buffer_end_);
      data_end_ += i;
    }

    /* Add a packet to this packet chain.  */
    void chain(Packet_ptr p) noexcept {
      if (!chain_) {
        chain_ = std::move(p);
        last_ = chain_.get();
      } else {
        auto* ptr = p.get();
        last_->chain(std::move(p));
        last_ = ptr->last_in_chain() ? ptr->last_in_chain() : ptr;
        assert(last_);
      }
    }

    /* Get the last packet in the chain */
    Packet* last_in_chain() noexcept
    { return last_; }

    /* Get the tail, i.e. chain minus the first element */
    Packet* tail() noexcept
    { return chain_.get(); }

    /* Get the tail, and detach it from the head (for FIFO) */
    Packet_ptr detach_tail() noexcept
    { return std::move(chain_); }


    // override delete to do nothing
    static void operator delete (void*) {}

  private:
    Packet_ptr chain_ {nullptr};
    Packet*    last_  {nullptr};

    /** Set layer begin, e.g. view the packet from another layer */
    void set_layer_begin(Byte_ptr loc)
    {
      Expects(loc >= buf() and loc <= buffer_end_);
      layer_begin_ = loc;
      // prevent data_end from being below layer_begin,
      // but also make sure its not moved back when decrementing
      data_end_    = std::max(loc, data_end_);
    }


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

    // const uint16_t     capacity_;
    Byte_ptr              layer_begin_;
    Byte_ptr              data_end_;
    const Byte* const     buffer_end_;
    BufferStore*          bufstore_;
    Byte buf_[0];
  }; //< class Packet

} //< namespace net

#endif
