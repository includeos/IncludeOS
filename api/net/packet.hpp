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
    // no-op destructor, see delete
    ~Packet() {}

    /** Get the buffer */
    Byte_ptr buf() noexcept
    { return &buf_[0]; }

    const Byte* buf() const noexcept
    { return &buf_[0]; }

    /** Get the start of the layer currently being accessed
     *  Returns a pointer to the start of the header
     */
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

    /** Get the total size of current layers data portion, >= size() and MTU-like */
    int capacity() const noexcept
    { return buffer_end_ - layer_begin_; }

    /** Returns the total packet data capacity, irrespective of layer */
    int bufsize() const noexcept
    { return buffer_end() - buf(); }

    /** Increment / decrement layer_begin, resets data_end */
    void increment_layer_begin(int i)
    {
      set_layer_begin(layer_begin() + i);
    }

    /** Modify/retrieve payload offset, used by higher levelprotocols */
    void set_payload_offset(int offset) noexcept
    {
      Expects(offset >= 0 and layer_begin() + offset < buffer_end());
      this->payload_off_ = layer_begin() + offset;
    }
    void increment_payload_offset(int offset) noexcept {
      this->payload_off_ += offset;
      Expects(payload_off_ >= layer_begin() and payload_off_ < buffer_end());
    }
    Byte_ptr payload() const noexcept {
      return this->payload_off_;
    }
    int payload_length() const noexcept {
      return this->data_end() - payload();
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

    /* Add a packet to this packet chain */
    inline void chain(Packet_ptr p) noexcept;

    /* Count packets in chain */
    inline int chain_length() const noexcept;

    /* Get the last packet in the chain */
    Packet* last_in_chain() noexcept
    { return last_; }

    /* Get the tail, i.e. chain minus the first element */
    Packet* tail() noexcept
    { return chain_.get(); }

    /* Get the tail, and detach it from the head (for FIFO) */
    Packet_ptr detach_tail() noexcept
    { return std::move(chain_); }


    // delete: release data back to buffer store
    // alternatively, free array of bytes if no bufferstore was set
    static void operator delete (void* data) {
      auto* pk = (Packet*) data;
      if (pk->bufstore_)
        pk->bufstore_->release(data);
      else
        delete[] (uint8_t*) data;
    }

  private:
    /** Set layer begin, e.g. view the packet from another layer */
    void set_layer_begin(Byte_ptr loc)
    {
      Expects(loc >= buf() and loc <= buffer_end_);
      layer_begin_ = loc;
      // prevent data_end from being below layer_begin,
      // but also make sure its not moved back when decrementing
      data_end_    = std::max(loc, data_end_);
    }


    Packet() = delete;
    Packet(Packet&) = delete;
    Packet(Packet&&) = delete;
    Packet& operator=(Packet) = delete;
    Packet operator=(Packet&&) = delete;

    Byte_ptr              layer_begin_;
    Byte_ptr              data_end_;
    Byte_ptr              payload_off_ = 0;
    const Byte* const     buffer_end_;

    Packet_ptr chain_ = nullptr;
    Packet*    last_  = nullptr;

    BufferStore*          bufstore_;
    Byte buf_[0];
  }; //< class Packet

  void Packet::chain(Packet_ptr pkt) noexcept
  {
    assert(pkt.get() != nullptr);
    assert(pkt.get() != this);

    auto* p = this;
    while (p->chain_ != nullptr) {
      p = p->chain_.get();
      assert(pkt.get() != p);
    }
    p->chain_ = std::move(pkt);

    /*
    if (!chain_) {
      chain_ = std::move(p);
      last_ = chain_.get();
    } else {
      auto* ptr = p.get();
      last_->chain(std::move(p));
      last_ = ptr->last_in_chain() ? ptr->last_in_chain() : ptr;
      assert(last_);
    }
    */
  }

  int Packet::chain_length() const noexcept
  {
    int count = 1;
    auto* p = this;
    while (p->chain_) {
      p = p->chain_.get();
      count++;
    }
    return count;
  }

} //< namespace net

#endif
