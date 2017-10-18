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

#include <common>
#include <stdexcept>
#include <vector>
#include <smp>

namespace net
{
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
    struct buffer_t
    {
      BufferStore* bufstore;
      uint8_t*     addr;
    };

    BufferStore(size_t num, size_t bufsize);
    ~BufferStore();

    buffer_t get_buffer();

    inline void release(void*);

    size_t local_buffers() const noexcept
    { return poolsize_ / bufsize_; }

    /** Get size of a buffer **/
    size_t bufsize() const noexcept
    { return bufsize_; }

    size_t poolsize() const noexcept
    { return poolsize_; }

    /** Check if a buffer belongs here */
    bool is_from_this_pool(uint8_t* addr) const noexcept {
      return (addr >= this->pool_begin()
          and addr <  this->pool_end());
    }

    /** Check if an address is the start of a buffer */
    bool is_buffer(uint8_t* addr) const noexcept
    { return (addr - pool_) % bufsize_ == 0; }

    size_t available() const noexcept;

    size_t total_buffers() const noexcept;

    /** move this bufferstore to the current CPU **/
    void move_to_this_cpu() noexcept;

  private:
    uint8_t* pool_begin() const noexcept {
      return pool_;
    }
    uint8_t* pool_end() const noexcept {
      return pool_begin() + poolsize_;
    }

    BufferStore* get_next_bufstore();
    inline buffer_t get_buffer_directly() noexcept;
    inline void     release_directly(uint8_t*);
    void release_internal(void*);

    size_t               poolsize_;
    size_t               bufsize_;
    uint8_t*             pool_;
    std::vector<uint8_t*> available_;
    BufferStore*         next_;
    int                  index;
#ifndef INCLUDEOS_SINGLE_THREADED
    // has strict alignment reqs, so put at end
    spinlock_t           plock = 0;
#endif
    BufferStore(BufferStore&)  = delete;
    BufferStore(BufferStore&&) = delete;
    BufferStore& operator=(BufferStore&)  = delete;
    BufferStore  operator=(BufferStore&&) = delete;
  };

  inline void BufferStore::release(void* addr)
  {
    auto* buff = (uint8_t*) addr;
    // try to release directly into pool
    if (LIKELY(is_from_this_pool(buff))) {
#ifndef INCLUDEOS_SINGLE_THREADED
      scoped_spinlock spinlock(this->plock);
#endif
      available_.push_back(buff);
      return;
    }
    // release via chained stores
    release_internal(addr);
  }

} //< net

#endif //< NET_BUFFER_STORE_HPP
