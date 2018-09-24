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

#include <cstdlib>
#include <net/buffer_store.hpp>
#include <os>
#include <common>
#include <cassert>
#include <smp>
#include <cstddef>
//#define DEBUG_RELEASE
//#define DEBUG_RETRIEVE
//#define DEBUG_BUFSTORE
#define ENABLE_BUFFERSTORE_GROWTH

#ifdef DEBUG_RELEASE
#define BSD_RELEASE(fmt, ...) printf(fmt, ##__VA_ARGS__);
#else
#define BSD_RELEASE(fmt, ...)  /** fmt **/
#endif

#ifdef DEBUG_RETRIEVE
#define BSD_GET(fmt, ...) printf(fmt, ##__VA_ARGS__);
#else
#define BSD_GET(fmt, ...)  /** fmt **/
#endif

#ifdef DEBUG_BUFSTORE
#define BSD_BUF(fmt, ...) printf(fmt, ##__VA_ARGS__);
#else
#define BSD_BUF(fmt, ...)  /** fmt **/
#endif

namespace net {

  BufferStore::BufferStore(size_t num, size_t bufsize) :
    poolsize_  {num * bufsize},
    bufsize_   {bufsize}
  {
    assert(num != 0);
    assert(bufsize != 0);
    available_.reserve(num);

    this->create_new_pool();
    assert(available_.capacity() == num);
    assert(available() == num);

    static int bsidx = 0;
    this->index = ++bsidx;
  }

  BufferStore::~BufferStore() {
    for (auto* pool : this->pools_)
        free(pool);
  }

  size_t BufferStore::available() const noexcept
  {
    return this->available_.size();
  }
  size_t BufferStore::total_buffers() const noexcept
  {
    return this->local_buffers() * this->pools_.size();
  }

  uint8_t* BufferStore::get_buffer()
  {
#ifdef INCLUDEOS_SMP_ENABLE
    scoped_spinlock spinlock(plock);
#endif

    if (UNLIKELY(available_.empty())) {
#ifdef ENABLE_BUFFERSTORE_GROWTH
      this->create_new_pool();
#else
      throw std::runtime_error("This BufferStore is empty");
#endif
    }
    
    auto* addr = available_.back();
    available_.pop_back();
    BSD_GET("%d: Gave away %p, %lu buffers remain\n",
            this->index, addr, available());
    return addr;
  }

  void BufferStore::create_new_pool()
  {
    auto* pool = (uint8_t*) aligned_alloc(OS::page_size(), poolsize_);
    assert(pool != nullptr);
    pools_.push_back(pool);

    for (uint8_t* b = pool; b < pool + poolsize_; b += bufsize_) {
        available_.push_back(b);
    }
  }

  void BufferStore::move_to_this_cpu() noexcept
  {
    // TODO: hmm
  }
  
  __attribute__((weak))
  bool BufferStore::growth_enabled() const {
    return true;
  }

} //< net
