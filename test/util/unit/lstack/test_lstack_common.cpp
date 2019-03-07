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

/*
  Separated test body to use with a couple of different lstack options set
 */

#include "test_lstack.hpp"

CASE("lstack::" STR(LSTACK_OPT) " basics") {
  using Lstack = alloc::Lstack<alloc::Lstack_opt::LSTACK_OPT>;
  Lstack heap;

  EXPECT(heap.allocation_end() == 0);
  EXPECT(heap.empty());
  EXPECT(heap.pool_size() == 0);
  EXPECT(heap.is_contiguous());

  size_t poolsize  = 0x100000;
  auto   blocksize = 0x1000;

  char* pool = (char*)memalign(blocksize, poolsize);
  void* pool_end = pool + poolsize;

  // Edge cases with empty pool
  EXPECT(heap.empty());
  EXPECT(heap.allocate(0) == nullptr);
  EXPECT(heap.allocate(blocksize) == nullptr);
  EXPECT(heap.allocate(poolsize) == nullptr);
  EXPECT(heap.allocate(blocksize + 1) == nullptr);
  EXPECT(heap.allocate(blocksize - 1) == nullptr);
  EXPECT(heap.allocate(poolsize  + 1) == nullptr);
  EXPECT(heap.allocate(poolsize  - 1) == nullptr);
  EXPECT(heap.allocate(std::numeric_limits<size_t>::max()) == nullptr);

  EXPECT(heap.allocate(rand() & ~4095) == nullptr);
  EXPECT(heap.allocate(rand() & ~4095) == nullptr);
  EXPECT(heap.allocate(rand() & ~4095) == nullptr);

  EXPECT_THROWS(heap.donate(nullptr, poolsize));
  EXPECT_THROWS(heap.donate(pool, 0));
  EXPECT_THROWS(heap.donate(pool, heap.min_alloc - 1));

  EXPECT(not heap.allocate_front(0));

  // Nothing should happen on dealloc of nullptr in e.g. POSIX free/*
  auto pre_null_dealloc = heap.bytes_free();
  heap.deallocate({nullptr, 0});
  heap.deallocate({nullptr, 4096});
  heap.deallocate(nullptr, 1);
  EXPECT(heap.bytes_free() == pre_null_dealloc);
  EXPECT(heap.empty());

  // Illegal dealloc edge cases
  // NOTE: Expect throws means we're supposed to hit a failed "Expects", not
  //       that we explicitly throw. Expects defaults to assert except in tests.

  EXPECT_THROWS(heap.deallocate({(void*)42, 42}));

  // Donate pool
  heap.donate(pool, poolsize);

  EXPECT(heap.bytes_allocated() == 0);
  EXPECT(heap.bytes_free() == poolsize);
  EXPECT(heap.allocation_end() == (uintptr_t)pool);
  EXPECT(heap.node_count() == 1);
  EXPECT(heap.pool_size() == poolsize);
  EXPECT(heap.is_contiguous());

  // Edge cases with non-empty pool
  EXPECT(not heap.empty());
  EXPECT(heap.allocate(0) == nullptr);
  EXPECT(heap.allocate(poolsize + 1) == nullptr);
  EXPECT(heap.allocate(poolsize + 42) == nullptr);
  EXPECT(heap.allocate(std::numeric_limits<size_t>::max()) == nullptr);
  EXPECT(not heap.allocate_front(0));

  heap.deallocate({nullptr, 0}); // no effect
  heap.deallocate(nullptr, 1);   // no effect
  EXPECT_THROWS(heap.deallocate({(void*)42, 42}));
  EXPECT_THROWS(heap.donate(pool, poolsize));
  print_summary(heap);

  EXPECT_THROWS(heap.donate(pool + 1, poolsize));
  EXPECT_THROWS(heap.donate(pool - 1, poolsize));
  EXPECT_THROWS(heap.donate(pool, poolsize + 1));
  EXPECT_THROWS(heap.donate(pool, poolsize - 1));
  EXPECT_THROWS(heap.donate(pool + 42, 42));
  EXPECT(heap.node_count() == 1);

  // Single alloc / dealloc
  size_t sz1 = 4096 * 2;
  auto* alloc1 = heap.allocate(sz1);

  EXPECT(heap.bytes_allocated() == sz1);
  EXPECT(heap.bytes_free() == poolsize - sz1);
  EXPECT(heap.allocation_end() == (uintptr_t)alloc1 + sz1);

  // Fail unaligned dealloc
  EXPECT_THROWS(heap.deallocate((void*)42, 42));

  // Fail dealloc outside pool
  EXPECT_THROWS(heap.deallocate((void*)4096, 4096));

  // Correct dealloc
  heap.deallocate(alloc1, sz1);
  EXPECT(heap.bytes_allocated() == 0);
  EXPECT(heap.bytes_free() == poolsize);

  // Survive allocation of size < alloc_min - should round up
  auto* small1 = heap.allocate(1337);
  EXPECT(small1 != nullptr);
  heap.deallocate(small1, 1337);
  EXPECT(heap.bytes_free()      == poolsize);
  EXPECT(heap.bytes_allocated() == 0);

  // Alloc functions returning struct can be used with C++17 structured bindings
  auto [data1, size1] = heap.allocate_front(42);

  EXPECT(typeid(decltype(data1)) == typeid(void*));
  EXPECT(typeid(decltype(size1)) == typeid(size_t));
  EXPECT(data1 == pool);
  EXPECT(size1 == heap.min_alloc);
  heap.deallocate({data1, size1});
  EXPECT(heap.bytes_allocated() == 0);


  if constexpr (heap.is_sorted) {
    EXPECT(heap.allocation_end() == (uintptr_t)pool);
  } else {
    EXPECT(heap.allocation_end() < (uintptr_t)pool_end);
  }

  // Allocate until empty
  std::vector<alloc::Allocation> allocs;
  uintptr_t alloc_max = 0;
  size_t alloc_sum = 0;
  while (heap.bytes_free() >= heap.min_alloc) {
    size_t size = std::max(bits::roundto(heap.align, heap.bytes_free() / 10), heap.min_alloc);
    auto old_use = heap.bytes_free();
    alloc::Allocation allocation{heap.allocate(size), size};
    alloc_sum += allocation.size;
    EXPECT(heap.bytes_free() == old_use - allocation.size);
    if ((uintptr_t)allocation.ptr + allocation.size > alloc_max) {
      alloc_max = (uintptr_t)allocation.ptr + allocation.size;
      EXPECT(heap.allocation_end() == alloc_max);
      print_summary(heap);
    }
    allocs.push_back(allocation);
  }

  EXPECT(heap.bytes_free() < heap.min_alloc);
  EXPECT(heap.allocate(4096) == nullptr);
  EXPECT(heap.allocate(0) == nullptr);
  EXPECT(alloc_sum == poolsize);
  print_summary(heap);
  EXPECT(heap.allocation_end() == (uintptr_t)pool + poolsize);

  bool highest_returned = false;
  // Free all
  for (auto alloc : allocs) {
    auto prev_avail = heap.bytes_free();
    heap.deallocate(alloc);
    alloc_sum -= alloc.size;

    if constexpr (heap.is_sorted) {
        if ((uintptr_t)alloc.ptr + alloc.size < heap.pool_end() and ! highest_returned) {
          EXPECT(heap.allocation_end() == heap.pool_end());
        }
    }
    EXPECT(heap.bytes_free() == prev_avail + alloc.size);
  }

  EXPECT(heap.bytes_free() == poolsize);
  EXPECT(heap.allocation_end() >= (uintptr_t)pool);

  allocs.clear();
  print_summary(heap);

  // Allocate until empty
  while (heap.bytes_free() >= heap.min_alloc) {
    size_t size = std::max(bits::roundto(heap.align, heap.bytes_free() / 100), heap.min_alloc);
    auto old_use = heap.bytes_free();
    alloc::Allocation allocation{heap.allocate(size),
        bits::roundto(heap.min_alloc, size)};
    EXPECT(heap.bytes_free() == old_use - allocation.size);
    if ((uintptr_t)allocation.ptr + allocation.size > alloc_max) {
      alloc_max = (uintptr_t)allocation.ptr + allocation.size;
      EXPECT(heap.allocation_end() == alloc_max);
    }
    allocs.push_back(allocation);
  }

  EXPECT(heap.bytes_free() < heap.min_alloc);
  EXPECT(heap.allocate(4096) == nullptr);
  EXPECT(heap.allocate(0) == nullptr);

  // Add secondary pool
  char* pool2 = (char*)memalign(blocksize, poolsize);
  heap.donate(pool2, poolsize);
  auto ch2 = heap.allocate(4096);
  EXPECT(ch2 != nullptr);
  EXPECT(heap.bytes_free() == poolsize - 4096);
  EXPECT(heap.bytes_allocated() == poolsize + 4096);

  for (auto alloc : allocs)
    heap.deallocate(alloc);

  EXPECT(heap.bytes_allocated() == 4096);
  heap.deallocate(ch2, 4096);
  EXPECT(heap.bytes_allocated() == 0);
  EXPECT(heap.bytes_free() == 2 * poolsize);
  EXPECT(! heap.is_contiguous());

  print_summary(heap);
  free(pool);
  free(pool2);
}

CASE("lstack::" STR(LSTACK_OPT) " fragmentation ") {
  using Alloc = alloc::Lstack<alloc::Lstack_opt::LSTACK_OPT>;
  test::pool<Alloc, 6 * 4_KiB> pool;
  Alloc heap = pool.stack;
  EXPECT(heap.bytes_free() == pool.size);

  auto chunksz = 4_KiB;

  // Allocate two even chunks
  auto* alloc1 = heap.allocate(chunksz);
  auto* alloc2 = heap.allocate(chunksz);
  auto* alloc3 = heap.allocate(chunksz);
  auto* alloc4 = heap.allocate(chunksz);
  auto* alloc5 = heap.allocate(chunksz);
  auto* alloc6 = heap.allocate(chunksz);

  EXPECT(heap.empty());
  EXPECT(alloc1 == pool.data);
  EXPECT((uintptr_t)alloc2 == pool.begin() + chunksz);
  EXPECT((uintptr_t)alloc3 == pool.begin() + 2 * chunksz);
  EXPECT((uintptr_t)alloc4 == pool.begin() + 3 * chunksz);
  EXPECT((uintptr_t)alloc5 == pool.begin() + 4 * chunksz);
  EXPECT((uintptr_t)alloc6 == pool.begin() + 5 * chunksz);

  EXPECT(heap.allocation_end() == pool.end());

  // Create non-mergeable holes
  // [][#][][][][]
  heap.deallocate(alloc2, chunksz);
  EXPECT(heap.allocation_end() == pool.end());
  EXPECT(heap.bytes_free() == chunksz);

  // [][#][][#][][]
  heap.deallocate(alloc4, chunksz);
  EXPECT(heap.allocation_end() == pool.end());
  EXPECT(heap.bytes_free() == chunksz * 2);

  // [][#][][#][][#]
  heap.deallocate(alloc6, chunksz);
  EXPECT(heap.allocation_end() == (uintptr_t)alloc6);
  EXPECT(heap.bytes_free() == chunksz * 3);

  // Fill holes
  // [][#][#][#][][#]
  heap.deallocate(alloc3, chunksz);
  EXPECT(heap.allocation_end() == (uintptr_t)alloc6);
  EXPECT(heap.bytes_free() == chunksz * 4);

  // [][#][#][#][#][#]
  heap.deallocate(alloc5, chunksz);
  EXPECT(heap.bytes_free() == chunksz * 5);

  if constexpr(heap.is_sorted){
    EXPECT(heap.allocation_end() == (uintptr_t)alloc2);
    auto lrg = heap.allocate_largest();
    EXPECT(lrg.ptr  == alloc2);
    EXPECT(lrg.size == chunksz * 5);
    EXPECT(heap.empty());
    EXPECT(heap.bytes_free() == 0);
    heap.deallocate(alloc2, lrg.size);
    EXPECT(heap.bytes_free() == chunksz * 5);
  } else {
    EXPECT(heap.allocation_end() == (uintptr_t)alloc6);
    EXPECT(heap.allocate_largest().ptr == alloc5);
    heap.deallocate(alloc5, chunksz);
  }

  heap.deallocate(alloc1, chunksz);
  EXPECT(heap.bytes_free() == pool.size);

  // Verify largest across fragments
  auto a1 = heap.allocate_front(chunksz);
  auto a2 = heap.allocate_front(chunksz);
  auto a3 = heap.allocate_front(chunksz * 2);
  auto a4 = heap.allocate_front(chunksz);
  auto a5 = heap.allocate_front(chunksz);

  if constexpr (heap.is_sorted) {
    EXPECT((a1 and a2 and a3 and a4 and a5));
    heap.deallocate(a1);
    heap.deallocate(a3);
    heap.deallocate(a5);
    EXPECT(heap.bytes_free() == a1.size + a3.size + a5.size);

    // Verify largest
    auto lrg1 = heap.allocate_largest();
    EXPECT(lrg1 == a3);
    EXPECT(heap.bytes_free() == a2.size + a5.size);
    auto lrg2 = heap.allocate_largest();
    EXPECT(lrg2 == a1);
    auto lrg3 = heap.allocate_largest();
    EXPECT(lrg3 == a5);

    EXPECT(heap.empty());

    heap.deallocate(a1);
    heap.deallocate(a2);
    heap.deallocate(a3);
    heap.deallocate(a4);
    heap.deallocate(a5);

    EXPECT(heap.node_count() == 1);

  } else {
    EXPECT((a1 and a2 and !a3 and a4 and a5));
    auto lrg1 = heap.allocate_largest();
    EXPECT(lrg1.size == chunksz);
    auto lrg2 = heap.allocate_largest();
    EXPECT(lrg2.size == chunksz);
    EXPECT(not heap.allocate_largest());

    EXPECT(heap.empty());

    heap.deallocate(a1);
    heap.deallocate(a2);
    heap.deallocate(a4);
    heap.deallocate(a5);
    heap.deallocate(lrg1);
    heap.deallocate(lrg2);
  }

  // Back to full pool
  EXPECT(heap.bytes_allocated() == 0);

  // Verify back-allocation across fragments
  // Create [#][][##][][#] where we want size [##] as high as possible
  a1 = heap.allocate_front(chunksz);
  a2 = heap.allocate_front(chunksz);
  a3 = heap.allocate_front(chunksz * 2);
  a4 = heap.allocate_front(chunksz);
  a5 = heap.allocate_front(chunksz);

  if constexpr (heap.is_sorted) {
    EXPECT((a1 and a2 and a3 and a4 and a5));

    // Make holes
    heap.deallocate(a1);
    heap.deallocate(a3);
    heap.deallocate(a5);

    EXPECT(a1.ptr < a3.ptr);
    EXPECT(a3.ptr < a5.ptr);
    EXPECT(heap.bytes_free() == a1.size + a3.size + a5.size);
    EXPECT(heap.node_count() == 3);

    // Verify back allocation
    auto lrg1 = heap.allocate_back(chunksz * 2);
    EXPECT(lrg1 == a3);
    EXPECT(heap.bytes_free() == a2.size + a5.size);
    EXPECT(heap.node_count() == 2);

    auto lrg2 = heap.allocate_back(chunksz);
    EXPECT(lrg2 == a5);
    EXPECT(heap.bytes_free() == a2.size);
    EXPECT(heap.node_count() == 1);

    auto lrg3 = heap.allocate_back(chunksz);
    EXPECT(lrg3 == a1);
    EXPECT(heap.empty());
    EXPECT(not heap.allocate_back(chunksz));

    heap.deallocate(a1);
    heap.deallocate(a2);
    heap.deallocate(a3);
    heap.deallocate(a4);
    heap.deallocate(a5);

  } else {
    EXPECT((a1 and a2 and !a3 and a4 and a5));

    auto lrg1 = heap.allocate_back(chunksz);
    EXPECT(lrg1.size == chunksz);
    EXPECT(lrg1.ptr  >  a5.ptr);

    auto lrg2 = heap.allocate_back(chunksz);
    EXPECT(lrg2.size == chunksz);
    EXPECT(lrg2.ptr  <  lrg1.ptr);

    EXPECT(heap.empty());
    EXPECT(not heap.allocate_back(chunksz));
    EXPECT(heap.empty());

    heap.deallocate(a1);
    heap.deallocate(a2);
    heap.deallocate(a4);
    heap.deallocate(a5);
    heap.deallocate(lrg1);
    heap.deallocate(lrg2);
    EXPECT(not heap.allocate_front(chunksz * 2));
  }

  // Back to full pool
  EXPECT(heap.bytes_allocated() == 0);
}

CASE("lstack::" STR(LSTACK_OPT) " allocate_front") {
  using Alloc = alloc::Lstack<alloc::Lstack_opt::LSTACK_OPT>;
  test::pool<Alloc, 1_MiB> pool;
  Alloc heap = pool.stack;

  auto chunksz = 4_KiB;
  std::vector<alloc::Allocation> allocs;
  size_t allocated = 0;

  while (! heap.empty()) {
    auto a = heap.allocate_front(chunksz);
    EXPECT(a.size);
    EXPECT(a.ptr);
    allocs.push_back(a);
    allocated += a.size;
    EXPECT(heap.bytes_free() == pool.size - allocated);
  }

  // Fail on unaligned deallocation
  EXPECT_THROWS(heap.deallocate((char*)allocs.front().ptr + 1, 16));

  for (auto a : allocs) {
    heap.deallocate(a);
    allocated -= a.size;
    EXPECT(heap.bytes_free() == pool.size - allocated);
  }

}

CASE("lstack::" STR(LSTACK_OPT) " small nodes") {
  using Alloc = alloc::Lstack<alloc::Lstack_opt::LSTACK_OPT, 16>;
  test::pool<Alloc, 1_MiB> pool;
  Alloc heap = pool.stack;
  EXPECT(heap.allocate(0) == nullptr);
  auto* alloc1 = heap.allocate(16);
  auto* alloc2 = heap.allocate(16);
  auto* alloc3 = heap.allocate(16);
  auto* alloc4 = heap.allocate(16);

  EXPECT(alloc1 != nullptr);
  EXPECT(alloc2 != nullptr);
  EXPECT(alloc3 != nullptr);
  EXPECT(alloc4 != nullptr);

  EXPECT(heap.bytes_free() == pool.size - 64);

  heap.deallocate(alloc1, 16);
  heap.deallocate(alloc2, 16);
  heap.deallocate(alloc3, 16);
  heap.deallocate(alloc4, 16);

  EXPECT(heap.bytes_free() == pool.size);
}

CASE("lstack::" STR(LSTACK_OPT) " allocate_back") {
  using Alloc = alloc::Lstack<alloc::Lstack_opt::LSTACK_OPT>;
  static constexpr auto chsz   = 4_KiB;
  static constexpr auto chunks = 6;
  test::pool<Alloc,chunks * chsz> pool;
  Alloc heap = pool.stack;
  EXPECT(heap.bytes_free() == pool.size);
  EXPECT(not heap.allocate_back(0));

  // Clean allocation back
  auto hi1 = heap.allocate_back(chsz);
  EXPECT(hi1.ptr == (void*)(pool.end() - chsz));
  EXPECT(hi1.size == chsz);
  EXPECT(heap.bytes_free() == pool.size - chsz);
  EXPECT(heap.allocation_end() == (uintptr_t)hi1.ptr + hi1.size);

  // Clean deallocation
  heap.deallocate(hi1);
  EXPECT(heap.bytes_free() == pool.size);
  if constexpr (heap.is_sorted) {
    EXPECT(heap.allocation_end() == pool.begin());
  } else {
    EXPECT(heap.allocation_end() == (uintptr_t)hi1.ptr);
  }

  // Allocation of unaligned size
  hi1 = heap.allocate_back(chsz - 7);
  EXPECT(hi1.ptr == (void*)(pool.end() - chsz));
  EXPECT(hi1.size == chsz);
  EXPECT(heap.bytes_free() == pool.size - chsz);
  EXPECT(heap.allocation_end() == (uintptr_t)hi1.ptr + hi1.size);

  // Deallocation of unaligned size
  heap.deallocate(hi1.ptr, chsz - 7);
  EXPECT(heap.bytes_free() == pool.size);
  if constexpr (heap.is_sorted) {
    EXPECT(heap.allocation_end() == pool.begin());
  } else {
    EXPECT(heap.allocation_end() == (uintptr_t)hi1.ptr);
  }

  // Too large allocation fails
  EXPECT(not heap.allocate_back(chsz * chunks + 7));

  // Pool is full again
  EXPECT(heap.bytes_allocated() == 0);


  // Deallocate all but last
  std::vector<alloc::Allocation> allocs;
  size_t allocated = 0;
  for (int i = 0; i < chunks - 1; i++) {
    auto a = heap.allocate_back(chsz);
    EXPECT(a.ptr  != nullptr);
    EXPECT(a.size != 0);
    EXPECT(heap.node_count() == 1);
    EXPECT(heap.bytes_allocated() == (i + 1) * chsz);
    EXPECT((uintptr_t)a.ptr == pool.end() - (i + 1) * chsz);
  }

  auto a = heap.allocate_back(chsz);
  EXPECT(a.ptr  != nullptr);
  EXPECT(a.size != 0);
  EXPECT(a.ptr == pool.data);
  EXPECT(heap.node_count() == 0);
  EXPECT(heap.empty());
}

CASE("lstack::" STR(LSTACK_OPT) " random allocs") {
  using namespace util;
  using Alloc = alloc::Lstack<alloc::Lstack_opt::LSTACK_OPT>;
  test::pool<Alloc, 1_MiB> pool;
  Alloc heap = pool.stack;
  EXPECT(heap.bytes_allocated() == 0);
  EXPECT(heap.bytes_free() == pool.size);
  EXPECT(heap.allocation_end() == (uintptr_t)pool.data);
  EXPECT(heap.allocation_begin() == (uintptr_t)pool.data);

  auto rnds = test::random;
  std::vector<alloc::Allocation> allocs;
  char data = 'A';
  for (auto rnd : rnds) {
    size_t r = std::max<size_t>(heap.min_alloc, rnd);
    if (heap.bytes_free() == 0)
      break;
    size_t sz = std::max(heap.min_alloc, r % heap.bytes_free());
    EXPECT(sz != 0);
    if (sz > heap.bytes_free())
      continue;

    auto aligned_size = bits::roundto(heap.align, sz);
    alloc::Allocation a{heap.allocate(sz), aligned_size};
    print_summary(heap);
    EXPECT(a.ptr != nullptr);
    EXPECT(a.size == aligned_size);
    EXPECT(((uintptr_t)a.ptr >= heap.pool_begin()
            and (uintptr_t)a.ptr < heap.pool_end()));
    allocs.push_back(a);
    std::memset(a.ptr, data, a.size);
    data++;
  }

  // TODO:
  // This means we've not done more than more allocation. That's extremely
  // unlikely but I guess it should be made impossible.
  EXPECT(data > 'A');

  size_t total_size = 0;
  for (auto a : allocs) {
    total_size += a.size;
  }

  // Deallocate all
  auto remaining = heap.bytes_free();
  EXPECT(remaining == pool.size - total_size);

  data = 'A';
  for (auto a : allocs) {
    // Verify data consistency
    char* c = (char*)a.ptr;
    std::string A (a.size, data);
    std::string B {(const char*)a.ptr, a.size};
    EXPECT(A == B);
    EXPECT(A.size() > 0);
    data++;

    // Deallocate and verify size
    heap.deallocate(a);
    EXPECT(heap.bytes_free() == remaining + a.size);
    remaining += a.size;
  }

  if constexpr (heap.is_sorted) {
    EXPECT(heap.node_count() == 1);
  }

  EXPECT(heap.bytes_free() == pool.size);
}

CASE("lstack::" STR(LSTACK_OPT) " make_unique") {
  using namespace util;
  using Alloc = alloc::Lstack<alloc::Lstack_opt::LSTACK_OPT>;
  test::pool<Alloc, 1_MiB> pool;
  Alloc heap = pool.stack;
  EXPECT(heap.bytes_free() == 1_MiB);
  auto uptr = heap.make_unique<test::pool<Alloc, 2_MiB>>();
  EXPECT(((uintptr_t)uptr.get() >= pool.begin() and (uintptr_t)uptr.get() < pool.end()));
  EXPECT(heap.bytes_free() < 1_MiB);
  uptr.reset();
  EXPECT(heap.bytes_free() == 1_MiB);
}
