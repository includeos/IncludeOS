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

#include <common.cxx>
#include <util/alloc_lstack.hpp>
#include <malloc.h>

using Lstack = util::alloc::Lstack<>;
using Chunk = Lstack::Chunk;

void print_summary(const Chunk* begin)
{
  #ifdef NO_INFO
  return;
  #else
  const Chunk* ptr = begin;
  if (ptr == nullptr) {
    printf("[0]\n");
    return;
  }

  Expects(ptr->size != 0);
  do {
    Expects(ptr != nullptr);
    printf("[");
    for (int i = 0; i < ptr->size / 4096; i++)
    {
        printf("#");
    }

    if (ptr->next == nullptr) {
      printf("]->0");
      break;
    } else {
      printf("]->");
    }
  } while((ptr = ptr->next));
  printf("\n");
  #endif
}

CASE("Using lstack")
{

  Lstack heap;
  EXPECT(heap.allocate(rand() & ~4095) == nullptr);

  auto poolsize = 0x100000;
  auto blocksize  = 0x1000;

  char* pool = (char*)memalign(blocksize, poolsize);
  void* pool_end = pool + poolsize;
  heap.donate(pool, poolsize);

  auto* page = heap.allocate(blocksize);
  EXPECT(page == pool);
  EXPECT(((Chunk*)page)->size == poolsize);
  EXPECT(((Chunk*)page)->next == nullptr);

  print_summary(heap.begin());

  int i = 0;
  for (; i < poolsize - blocksize; i += blocksize)
  {
    auto* p = heap.allocate(blocksize);
    EXPECT(p == (uint8_t*)pool + blocksize + i);
  }

  print_summary(heap.begin());

  EXPECT(heap.allocate(blocksize) == nullptr);
  EXPECT(heap.begin() == nullptr);

  char* chunk1 = pool + 0x1000;
  heap.deallocate(chunk1, 0x2000);
  EXPECT((char*)heap.begin() == chunk1);
  EXPECT((char*)heap.begin()->next == nullptr);

  print_summary(heap.begin());

  auto* chunk2 = pool + 0x5000;
  heap.deallocate(chunk2, 0x4000);
  EXPECT((char*)heap.begin() == chunk2);
  EXPECT((char*)heap.begin()->next == chunk1);
  EXPECT((char*)heap.begin()->next->next == nullptr);

  print_summary(heap.begin());
  EXPECT(heap.allocate(0x4000) == chunk2);
  print_summary(heap.begin());
  EXPECT(heap.allocate(0x1000) == chunk1);
  EXPECT(heap.allocate(0x1000) == chunk1 + 0x1000);
  EXPECT(heap.begin() == nullptr);

  // Free some small chunks in random order
  void* prev = nullptr;
  std::vector<int> rands;
  auto size  = blocksize * 2;
  auto count = poolsize / size;
  auto index = 0;

  for (i = 0; i < 5; i++)
  {

    do {
      index = rand() % (count - 1);
    } while (std::find(rands.begin(), rands.end(),  index) != rands.end());

    rands.push_back(index);

    auto* frag = &(pool)[index * size];
    EXPECT((frag >= pool && frag <= pool + poolsize - size));
    EXPECT(((uintptr_t)frag & blocksize - 1 ) == 0);
    heap.donate(frag, size);
    EXPECT((void*)heap.begin() == frag);
    prev = frag;
    print_summary(heap.begin());
  }

  print_summary(heap.begin());
  int blockcount = 0;
  int total_size = 0;
  auto* block = heap.begin();
  do {
    EXPECT(block != nullptr);
    EXPECT(((void*)block >= pool && (void*)block <= pool + poolsize));
    blockcount++;
    total_size += block->size;
    EXPECT(block->size == blocksize * 2);
  } while ((block = block->next));

  EXPECT(block == nullptr);
  EXPECT(blockcount == 5);
  EXPECT(total_size == 10 * blocksize);


  // Fragment the first chunk
  chunk1 = (char*)heap.allocate(0x1000);
  EXPECT(chunk1 == pool + (rands.back() * size));
  print_summary(heap.begin());
  heap.deallocate(chunk1, 0x1000);
  print_summary(heap.begin());
  EXPECT(heap.begin() == (Chunk*)chunk1);
  EXPECT(heap.begin()->next->size == 0x1000);
  EXPECT(heap.begin()->next->next->size == 0x2000);

  auto* first_2k = heap.begin()->next->next;
  chunk2 = (char*)heap.allocate(0x2000);
  print_summary(heap.begin());
  EXPECT((Chunk*)chunk2 == first_2k);

  free(pool);


}
