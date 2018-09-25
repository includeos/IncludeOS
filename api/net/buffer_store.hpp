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
    BufferStore(uint32_t num, uint32_t bufsize);
    ~BufferStore();

    uint8_t* get_buffer();

    inline void release(void*);

    /** Get size of a buffer **/
    uint32_t bufsize() const noexcept
    { return bufsize_; }

    uint32_t poolsize() const noexcept
    { return poolsize_; }

    /** Check if an address belongs to this buffer store */
    bool is_valid(uint8_t* addr) const noexcept
    {
      for (const auto* pool : pools_)
          if ((addr - pool) % bufsize_ == 0
            && addr >= pool && addr < pool + poolsize_) return true;
      return false;
    }

    size_t available() const noexcept {
      return this->available_.size();
    }

    size_t total_buffers() const noexcept {
      return this->pool_buffers() * this->pools_.size();
    }

    size_t buffers_in_use() const noexcept {
      return this->total_buffers() - this->available();
    }

    /** move this bufferstore to the current CPU **/
    void move_to_this_cpu() noexcept;

  private:
    uint32_t pool_buffers() const noexcept { return poolsize_ / bufsize_; }
    void create_new_pool();
    bool growth_enabled() const;

    uint32_t              poolsize_;
    uint32_t              bufsize_;
    int                   index = -1;
    std::vector<uint8_t*> available_;
    std::vector<uint8_t*> pools_;
#ifdef INCLUDEOS_SMP_ENABLE
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
    if (LIKELY(this->is_valid(buff))) {
#ifdef INCLUDEOS_SMP_ENABLE
      scoped_spinlock spinlock(this->plock);
#endif
      this->available_.push_back(buff);
      return;
    }
    throw std::runtime_error("Buffer did not belong");
  }

} //< net

#endif //< NET_BUFFER_STORE_HPP
