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

    BufferStore() = delete;
    BufferStore(size_t num, size_t bufsize);
    ~BufferStore();

    buffer_t get_buffer();
    void release(void*);

    /** Get size of a buffer **/
    size_t bufsize() const noexcept
    { return bufsize_; }

    size_t poolsize() const noexcept
    { return poolsize_; }

    /** Check if a buffer belongs here */
    bool is_from_pool(uint8_t* addr) const noexcept
    { return addr >= pool_begin() and addr < pool_end(); }

    /** Check if an address is the start of a buffer */
    bool is_buffer(uint8_t* addr) const noexcept
    { return (addr - pool_) % bufsize_ == 0; }

    size_t available() const noexcept
    { return available_.size(); }

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

    size_t               poolsize_;
    size_t               bufsize_;
    uint8_t*             pool_;
    std::vector<uint8_t*> available_;
    BufferStore*         next_;
    int                  cpu;
    static bool          smp_enabled_;
#ifndef INCLUDEOS_SINGLE_THREADED
    // has strict alignment reqs, so put at end
    spinlock_t           plock;
#endif
    BufferStore(BufferStore&)  = delete;
    BufferStore(BufferStore&&) = delete;
    BufferStore& operator=(BufferStore&)  = delete;
    BufferStore  operator=(BufferStore&&) = delete;
  };
} //< net

#endif //< NET_BUFFER_STORE_HPP
