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
#include <vector>
#include <cassert>
#include <malloc.h>
#include <cstdio>

#include <net/buffer_store.hpp>
#include <kernel/syscalls.hpp>
#define PAGE_SIZE     0x1000

#include <kernel/elf.hpp>

namespace net {

  BufferStore::BufferStore(size_t num, size_t bufsize) :
    poolsize_  {num * bufsize},
    bufsize_   {bufsize}
  {
    assert(num != 0);
    assert(bufsize != 0);

    const size_t DATA_SIZE  = poolsize_;
    const size_t BMP_CHUNKS = num / 32;
    const size_t BMP_SIZE   = BMP_CHUNKS * sizeof(uint32_t);

    this->pool_ = (buffer_t) memalign(PAGE_SIZE, DATA_SIZE + BMP_SIZE);
    assert(this->pool_);

    available_.reserve(num);
    for (buffer_t b = pool_end()-bufsize; b >= pool_begin(); b -= bufsize) {
        available_.push_back(b);
    }
    assert(available() == num);
    // verify that the "first" buffer is the start of the pool
    assert(available_.back() == pool_);

    new (&locked) MemBitmap((char*) pool_ + DATA_SIZE, BMP_CHUNKS);
    locked.zero_all();

    //printf("strtab: %p  end: %p\n", Elf::get_strtab(), Elf::get_strtab() + Elf::get_strtab_size());
    //printf("vec: %p end: %p vec data: %p\n", &available_, ((char*) &available_) + sizeof(available_), available_.data());
    //printf("bmp: %p end: %p\n", &locked, ((char*) &locked) + sizeof(MemBitmap));
  }

  BufferStore::~BufferStore() {
    free(this->pool_);
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
