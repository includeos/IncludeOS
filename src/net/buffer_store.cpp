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
#include <vector>
#include <cassert>
#include <malloc.h>
#include <cstdio>

#include <net/buffer_store.hpp>
#include <kernel/syscalls.hpp>
#include <debug>
#define PAGE_SIZE     0x1000

namespace net {

  BufferStore::BufferStore(size_t num, size_t bufsize) :
    poolsize_      {num * bufsize},
    bufsize_       {bufsize}
  {
    pool_ = (buffer_t) memalign(PAGE_SIZE, num * bufsize);
    assert(pool_);

    for (buffer_t b = pool_begin(); b < pool_end(); b += bufsize)
        available_.push_back(b);
    assert(available() == num);

    locked_storage = new uint32_t[num / 32];
    new (&locked) MemBitmap(locked_storage, num / 32);
    locked.zero_all();
  }

  BufferStore::~BufferStore() {
    free(pool_);
    delete[] locked_storage;
  }

  BufferStore::buffer_t BufferStore::get_buffer() {
    if (available_.empty())
      panic("<BufferStore> Storage pool full! Don't know how to increase pool size yet.\n");

    auto addr = available_.back();
    available_.pop_back();
    return addr;
  }

  void BufferStore::release(buffer_t addr)
  {
    debug("Release %p...", addr);
    if (is_from_pool(addr) and is_buffer(addr)) {
      // if the buffer is locked, don't release it
      if (locked.get( buffer_id(addr) )) {
        debug(" .. but it was locked\n");
        return;
      }
      
      available_.push_back(addr);
      debug("released\n");
      return;
    }
    // buffer not owned by bufferstore, so just delete it?
    debug("deleted\n");
    delete[] addr;
  }
  void BufferStore::unlock_and_release(buffer_t addr)
  {
    debug("Unlock and release %p...", addr);
    if (is_from_pool(addr) and is_buffer(addr)) {
      locked.reset( buffer_id(addr) );
      available_.push_back(addr);
      debug("released\n");
      return;
    }
    // buffer not owned by bufferstore, so just delete it?
    debug("deleted\n");
    delete[] addr;
  }

} //< namespace net
