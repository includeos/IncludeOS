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

//#define DEBUG
//#undef  NO_DEBUG
#if !defined(__MACH__)
#include <malloc.h>
#else
#include <cstddef>
extern void *memalign(size_t, size_t);
#endif
#include <net/buffer_store.hpp>
#include <kernel/syscalls.hpp>
#include <common>
#include <debug>
#include <info>
#include <cassert>
#include <smp>
#define PAGE_SIZE     0x1000

#define ENABLE_BUFFERSTORE_CHAIN
#define BS_CHAIN_ALLOC_PACKETS   2048

namespace net {

  bool BufferStore::smp_enabled_ = false;

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
    assert(available() == num);

#ifndef INCLUDEOS_SINGLE_THREADED
    // set CPU id this bufferstore was created for
    this->cpu = SMP::cpu_id();
    if (this->cpu != 0) smp_enabled_ = true;
#else
    this->cpu = 0;
#endif
  }

  BufferStore::~BufferStore() {
    delete this->next_;
    free(this->pool_);
  }

  BufferStore* BufferStore::get_next_bufstore()
  {
#ifdef ENABLE_BUFFERSTORE_CHAIN
    BufferStore* parent = this;
    while (parent->next_ != nullptr) {
        parent = parent->next_;
        if (parent->available() != 0) return parent;
    }
    INFO("BufferStore", "Allocating %u new packets", BS_CHAIN_ALLOC_PACKETS);
    parent->next_ = new BufferStore(BS_CHAIN_ALLOC_PACKETS, bufsize());
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
    bool is_locked = false;
    if (smp_enabled_) {
      lock(plock);
      is_locked = true;
    }
#endif

    if (UNLIKELY(available_.empty())) {
#ifndef INCLUDEOS_SINGLE_THREADED
      if (is_locked) unlock(plock);
#endif
#ifdef ENABLE_BUFFERSTORE_CHAIN
      return get_next_bufstore()->get_buffer_directly();
#else
      panic("<BufferStore> Buffer pool empty! Not configured to increase pool size.\n");
#endif
    }

    auto addr = available_.back();
    available_.pop_back();

#ifndef INCLUDEOS_SINGLE_THREADED
    if (is_locked) unlock(plock);
#endif
    return { this, addr };
  }

  void BufferStore::release(void* addr)
  {
    auto* buff = (uint8_t*) addr;
    debug("Release %p -> ", buff);

#ifndef INCLUDEOS_SINGLE_THREADED
    bool is_locked = false;
    if (smp_enabled_) {
      lock(plock);
      is_locked = true;
    }
#endif
    // expensive: is_buffer(buff)
    if (LIKELY(is_from_pool(buff))) {
      available_.push_back(buff);
#ifndef INCLUDEOS_SINGLE_THREADED
      if (is_locked) unlock(plock);
#endif
      debug("released\n");
      return;
    }
#ifdef ENABLE_BUFFERSTORE_CHAIN
    // try to release buffer on linked bufferstore
    BufferStore* ptr = next_;
    while (ptr != nullptr) {
      if (ptr->is_from_pool(buff)) {
        debug("released on other bufferstore\n");
        ptr->release_directly(buff);
        return;
      }
      ptr = ptr->next_;
    }
#endif
    // buffer not owned by bufferstore, so just delete it?
    debug("deleted\n");
    delete[] buff;
#ifndef INCLUDEOS_SINGLE_THREADED
    if (is_locked) unlock(plock);
#endif
  }

  void BufferStore::release_directly(uint8_t* buffer)
  {
    available_.push_back(buffer);
  }

  void BufferStore::move_to_this_cpu() noexcept
  {
    this->cpu = SMP::cpu_id();
    if (this->cpu != 0) smp_enabled_ = true;
  }

} //< namespace net
