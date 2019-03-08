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
#include <kernel/memory.hpp>
#include <common>
#include <cassert>
#include <smp>
#include <cstddef>
#ifdef __MACH__
extern void* aligned_alloc(size_t alignment, size_t size);
#endif
//#define DEBUG_BUFSTORE

#ifdef DEBUG_BUFSTORE
#define BSD_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__);
#else
#define BSD_PRINT(fmt, ...)  /** fmt **/
#endif

namespace net {

  BufferStore::BufferStore(uint32_t num, uint32_t bufsize) :
    poolsize_  {num * bufsize},
    bufsize_   {bufsize}
  {
    assert(num != 0);
    assert(bufsize != 0);
    available_.reserve(num);

    this->create_new_pool();
    assert(this->available_.capacity() == num);
    assert(available() == num);

    static int bsidx = 0;
    this->index = ++bsidx;
  }

  BufferStore::~BufferStore() {
    for (auto* pool : this->pools_)
        free(pool);
  }

  uint8_t* BufferStore::get_buffer()
  {
#ifdef INCLUDEOS_SMP_ENABLE
    scoped_spinlock spinlock(this->plock);
#endif

    if (UNLIKELY(available_.empty())) {
      if (this->growth_enabled())
          this->create_new_pool();
      else
          throw std::runtime_error("This BufferStore has run out of buffers");
    }

    auto* addr = available_.back();
    available_.pop_back();
    BSD_PRINT("%d: Gave away %p, %zu buffers remain\n",
            this->index, addr, available());
    return addr;
  }

  void BufferStore::create_new_pool()
  {
    auto* pool = (uint8_t*) aligned_alloc(os::mem::min_psize(), poolsize_);
    if (UNLIKELY(pool == nullptr)) {
      throw std::runtime_error("Buffer store failed to allocate memory");
    }
    this->pools_.push_back(pool);

    for (uint8_t* b = pool; b < pool + poolsize_; b += bufsize_) {
        this->available_.push_back(b);
    }
    BSD_PRINT("%d: Creating new pool, now %zu total buffers\n",
              this->index, this->total_buffers());
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
