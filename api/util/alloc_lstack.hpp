// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

#ifndef UTIL_MINIALLOC_HPP
#define UTIL_MINIALLOC_HPP

#include <common>

namespace util {
namespace alloc {

/**
 * Lazy stack of memory blocks.
 * A stack-like data structure for arbitrarily sized blocks.
 * - Uses free blocks as storage for the nodes (no memory overhead). This also
 *   implies that the first part of each free block must be present in memory.
 * - Lazily chops smaller blocks off of larger blocks as requested.
 * - Constant time first time allocation of any sized block.
 * - LIFO stack behavior for reallocating uniformly sized blocks (pop)
 * - Constant time deallocation (push).
 * - Linear search, first fit, for reallocatiing arbitrarily sized blocks.
 *   The first block larger than the requested size will be chopped to match,
 *   which implies that the size of the blocks will converge on Min and the
 *   number of blocks will converge on total size / Min for arbitrarily sized
 *   allocations.
 *
 * The allocator *can* be used as a general purpose allocator, but will be slow
 * for that purpose. E.g. after n arbitrarily sized deallocations the complexity
 * of allocating a block of size k will be O(n).
 *
 * NOTE: The allocator will not search for duplicates on deallocation,
 *       e.g. won't prevent double free, so that has to be done elsewhere.
 **/
template <ssize_t Min = 4096>
class Lstack {
public:

  struct Chunk {
    Chunk* next;
    size_t size;
    Chunk(Chunk* n, size_t s)
      : next(n), size(s)
    {
      Expects(s >= Min);
    }
  };

  static constexpr int align = Min;

  void* allocate(ssize_t size){
    Expects((size & (Min - 1)) == 0);

    Chunk* next = front_;
    Chunk** prev = &front_;
    do {

      if (UNLIKELY(next == nullptr))
        return nullptr;

      // Cleanly take out chunk as-is
      if (next->size == size)
      {
        *prev = next->next;
        return next;
      }

      // Clip off size from chunk
      if ( next->size > size)
      {
        if (next->size - size <= 0 and next->next == nullptr)
          *prev = nullptr;
        else
          *prev = new_chunk((char*)next + size, next->next, next->size - size);
        return next;
      }

      prev = &(next->next);
    } while ((next = next->next));

    // No suitable chunk
    return nullptr;
  }

  void deallocate (void* ptr, size_t size){
    push(ptr, size);
  }

  void donate(void* ptr, ssize_t size){
    push(ptr, size);
  }

  void push(void* ptr, ssize_t size){
    Expects((size & (align - 1)) == 0);
    auto* old_front = front_;
    front_ = (Chunk*)ptr;
    new_chunk(front_, old_front, size);
  };

  const Chunk* begin() const {
    return front_;
  }

  bool empty() const noexcept { return front_ == nullptr || front_->size == 0; }

private:
  Chunk* new_chunk(void* addr_begin, Chunk* next, size_t sz){
    Expects((sz & align - 1) == 0);
    Expects(((uintptr_t)addr_begin & align - 1) == 0);
    return new ((Chunk*)addr_begin) Chunk(next, sz);
  }
  Chunk* front_ = nullptr;
};

} // namespace util
} // namespace alloc
#endif
