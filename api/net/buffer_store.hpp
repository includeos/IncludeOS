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
#ifndef NET_BUFFER_STORE_HPP
#define NET_BUFFER_STORE_HPP

#include <stdexcept>
#include <vector>
#include <net/inet_common.hpp>
#include <util/membitmap.hpp>

namespace net{

  /**
   * Network buffer storage for uniformly sized buffers.
   *
   * @note : The buffer store is intended to be used by Packet, which is
   * a semi-intelligent buffer wrapper, used throughout the IP-stack.
   *
   * There shouldn't be any need for raw buffers in services.
   **/
  class BufferStore {
  public:
    using buffer_t = uint8_t*;

    BufferStore() = delete;
    BufferStore(size_t num, size_t bufsize);
    ~BufferStore();

    buffer_t get_buffer();
    void release(void*);

    /** Get size of a buffer **/
    inline size_t bufsize() const noexcept
    { return bufsize_; }

    inline size_t poolsize() const noexcept
    { return poolsize_; }

    /** Check if a buffer belongs here */
    inline bool is_from_pool(buffer_t addr) const noexcept
    { return addr >= pool_begin() and addr < pool_end(); }

    /** Check if an address is the start of a buffer */
    inline bool is_buffer(buffer_t addr) const noexcept
    { return (addr - pool_) % bufsize_ == 0; }

    inline size_t available() const noexcept
    { return available_.size(); }
    
    void lock(void* addr) {
      auto* buffer = (buffer_t) addr;
      assert(is_from_pool(buffer));
      locked.set( buffer_id(buffer) );
    }
    void unlock_and_release(buffer_t addr);

  private:
    buffer_t pool_begin() const noexcept {
      return pool_;
    }
    buffer_t pool_end() const noexcept {
      return pool_begin() + poolsize_;
    }
    size_t buffer_id(buffer_t addr) const {
      return (addr - pool_) / bufsize_;
    }
    
    size_t               poolsize_;
    const size_t         bufsize_;
    buffer_t             pool_;
    std::vector<buffer_t> available_;
    MemBitmap  locked;

    BufferStore(BufferStore&)  = delete;
    BufferStore(BufferStore&&) = delete;
    BufferStore& operator=(BufferStore&)  = delete;
    BufferStore  operator=(BufferStore&&) = delete;
  }; //< class BufferStore
} //< namespace net

#endif //< NET_BUFFER_STORE_HPP
