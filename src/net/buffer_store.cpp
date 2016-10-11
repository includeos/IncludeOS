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
#include <debug>
#include <cassert>
#include <malloc.h>
#include <cstdio>

#include <net/buffer_store.hpp>
#include <net/packet.hpp>
#include <kernel/syscalls.hpp>
#define PAGE_SIZE     0x1000

namespace net {

  Packet::~Packet()
  {
    if (bufstore)
        bufstore->release(this);
    else
        delete[] (uint8_t*) this;
  }

  BufferStore::BufferStore(size_t num, size_t bufsize) :
    poolsize_  {num * bufsize},
    bufsize_   {bufsize}
  {
    assert(num != 0);
    assert(bufsize != 0);
    const size_t DATA_SIZE  = poolsize_;

    this->pool_ = (buffer_t) memalign(PAGE_SIZE, DATA_SIZE);
    assert(this->pool_);

    available_.reserve(num);
    for (buffer_t b = pool_end()-bufsize; b >= pool_begin(); b -= bufsize) {
        available_.push_back(b);
    }
    assert(available() == num);
    // verify that the "first" buffer is the start of the pool
    assert(available_.back() == pool_);
  }

  BufferStore::~BufferStore() {
    free(this->pool_);
  }

  BufferStore::buffer_t BufferStore::get_buffer() {
    if (UNLIKELY(available_.empty())) {
      panic("<BufferStore> Storage pool full! Don't know how to increase pool size yet.\n");
    }

    auto addr = available_.back();
    available_.pop_back();
    return addr;
  }

  void BufferStore::release(void* addr)
  {
    buffer_t buff = (buffer_t) addr;
    debug("Release %p...", buff);

    // expensive: is_buffer(buff)
    if (LIKELY(is_from_pool(buff))) {
      available_.push_back(buff);
      debug("released\n");
      return;
    }
    // buffer not owned by bufferstore, so just delete it?
    debug("deleted\n");
    delete[] buff;
  }

} //< namespace net
