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

#if !defined(__MACH__)
#include <malloc.h>
#else
#include <cstddef>
extern void *memalign(size_t, size_t);
#endif
#include <net/buffer_store.hpp>
#include <kernel/syscalls.hpp>
#include <common>
#include <cassert>
#include <smp>
#include <cstddef>
//#define DEBUG_RELEASE
//#define DEBUG_RETRIEVE
//#define DEBUG_BUFSTORE

#define PAGE_SIZE     0x1000
#define ENABLE_BUFFERSTORE_CHAIN


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
    bufsize_   {bufsize},
    next_(nullptr)
  {
    assert(num != 0);
    assert(bufsize != 0);
    const size_t DATA_SIZE  = poolsize_;

    this->pool_ = (uint8_t*) memalign(PAGE_SIZE, DATA_SIZE);
    assert(this->pool_);

    available_.reserve(num);
    for (uint8_t* b = pool_end()-bufsize; b >= pool_begin(); b -= bufsize) {
        available_.push_back(b);
    }
    assert(available_.capacity() == num);
    assert(available() == num);

    static int bsidx = 0;
    this->index = ++bsidx;
  }

  BufferStore::~BufferStore() {
    delete this->next_;
    free(this->pool_);
  }

  size_t BufferStore::available() const noexcept
  {
    auto avail = this->available_.size();
#ifdef ENABLE_BUFFERSTORE_CHAIN
    auto* parent = this;
    while (parent->next_ != nullptr) {
        parent = parent->next_;
        avail += parent->available_.size();
    }
#endif
    return avail;
  }
  size_t BufferStore::total_buffers() const noexcept
  {
    size_t total = this->local_buffers();
#ifdef ENABLE_BUFFERSTORE_CHAIN
    auto* parent = this;
    while (parent->next_ != nullptr) {
        parent = parent->next_;
        total += parent->local_buffers();
    }
#endif
    return total;
  }

  BufferStore* BufferStore::get_next_bufstore()
  {
#ifdef ENABLE_BUFFERSTORE_CHAIN
    BufferStore* parent = this;
    while (parent->next_ != nullptr) {
        parent = parent->next_;
        if (!parent->available_.empty()) return parent;
    }
    parent->next_ = new BufferStore(local_buffers(), bufsize());
    BSD_BUF("<BufferStore> Allocating %lu new buffers (%lu total)",
            local_buffers(), total_buffers());
    return parent->next_;
#else
    return nullptr;
#endif
  }

  BufferStore::buffer_t BufferStore::get_buffer_directly() noexcept
  {
    auto addr = available_.back();
    available_.pop_back();
    return { this, addr };
  }

  BufferStore::buffer_t BufferStore::get_buffer()
  {
#ifndef INCLUDEOS_SINGLE_THREADED
    scoped_spinlock spinlock(plock);
#endif

    if (UNLIKELY(available_.empty())) {
#ifdef ENABLE_BUFFERSTORE_CHAIN
      auto* next = get_next_bufstore();
      if (next == nullptr)
          throw std::runtime_error("Unable to create new bufstore");

      // take buffer from external bufstore
      auto buffer = next->get_buffer_directly();
      BSD_GET("%d: Gave away EXTERN %p, %lu buffers remain\n",
              this->index, buffer.addr, available());
      return buffer;
#else
      panic("<BufferStore> Buffer pool empty! Not configured to increase pool size.\n");
#endif
    }

    auto buffer = get_buffer_directly();
    BSD_GET("%d: Gave away %p, %lu buffers remain\n",
            this->index, buffer.addr, available());
    return buffer;
  }

  void BufferStore::release_internal(void* addr)
  {
    auto* buff = (uint8_t*) addr;
    BSD_RELEASE("%d: Release %p -> ", this->index, buff);

#ifndef INCLUDEOS_SINGLE_THREADED
    scoped_spinlock spinlock(plock);
#endif
#ifdef ENABLE_BUFFERSTORE_CHAIN
    // try to release buffer on linked bufferstore
    BufferStore* ptr = next_;
    while (ptr != nullptr) {
      if (ptr->is_from_this_pool(buff)) {
        BSD_RELEASE("released on other bufferstore\n");
        ptr->release_directly(buff);
        return;
      }
      ptr = ptr->next_;
    }
#endif
    throw std::runtime_error("Packet did not belong");
  }

  void BufferStore::release_directly(uint8_t* buffer)
  {
    BSD_GET("%d: Released EXTERN %p, %lu buffers remain\n",
            this->index, buffer, available());
    available_.push_back(buffer);
  }

  void BufferStore::move_to_this_cpu() noexcept
  {
    // TODO: hmm
  }

} //< namespace net
