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

#include "test_lstack.hpp"

CASE("lstack::nodes:no_merge: testing empty lstack")
{
  using Alloc = util::alloc::detail::Lstack<util::alloc::Lstack_opt::no_merge>;
  Alloc heap;
  EXPECT(*heap.find_largest() == nullptr);
  EXPECT(heap.node_count() == 0);
  EXPECT(heap.find_prior(0) == nullptr);
  EXPECT(heap.find_prior((void*)1) == nullptr);
  EXPECT(heap.find_prior((void*)4096) == nullptr);
  EXPECT(heap.find_prior((void*)(4096 * 1024)) == nullptr);
  EXPECT_THROWS(heap.pop(nullptr));
  Alloc::Node* ptr = nullptr;
  EXPECT((heap.pop(&ptr) == util::alloc::Allocation{}));
  EXPECT(heap.pop_off(0) == nullptr);
  EXPECT(heap.pop_off(1) == nullptr);
  EXPECT(heap.pop_off(4906) == nullptr);
  EXPECT(heap.pop_off(4906 * 1024) == nullptr);
  EXPECT(heap.allocation_begin() == nullptr);
}

CASE("lstack::nodes: testing lstack<no_merge> node traversal")
{
  using namespace util::literals;
  using Alloc = util::alloc::detail::Lstack<util::alloc::Lstack_opt::no_merge>;
  using Node  = util::alloc::Node<>;
  test::pool<Alloc, 1_MiB> pool;
  Alloc heap = pool.stack;
  EXPECT(heap.bytes_allocated() == 0);
  EXPECT(heap.bytes_free() == pool.size);
  EXPECT(heap.allocation_end() == (uintptr_t)pool.data);
  EXPECT(heap.allocation_begin() == (Node*)pool.data);

  // Find largest
  auto* first_begin = heap.allocation_begin();
  EXPECT(*(heap.find_largest()) == heap.allocation_begin());

  auto sz = 4_KiB;

  // Stack behavior - get the same chunk every time
  heap.deallocate(heap.allocate(sz), sz);
  heap.deallocate(heap.allocate(sz), sz);
  heap.deallocate(heap.allocate(sz), sz);
  heap.deallocate(heap.allocate(sz), sz);

  // No merging enabled
  EXPECT(heap.node_count() == 2);

  EXPECT((uintptr_t)(*(heap.find_largest())) == (uintptr_t)((char*)first_begin + sz));
  print_summary(heap);
  auto pop1 = heap.pop_off(sz);
  EXPECT(pop1 == first_begin);
  EXPECT((uintptr_t)(*(heap.find_largest())) == (uintptr_t)((char*)first_begin + sz));
  EXPECT(heap.node_count() == 1);

  print_summary(heap);

  // Create gap
  auto sz2 = sz * 2;
  auto pop2 = heap.pop_off(sz2);
  EXPECT((uintptr_t)pop2 == (uintptr_t)first_begin + sz);
  EXPECT(heap.node_count() == 1);

  auto sz3 = sz * 3;
  auto pop3 = heap.pop_off(sz3);
  EXPECT((uintptr_t)pop3 == (uintptr_t)first_begin + sz + sz2);
  EXPECT(heap.node_count() == 1);

  auto sz4 = sz * 4;
  auto pop4 = heap.pop_off(sz4);
  EXPECT((uintptr_t)pop4 == (uintptr_t)first_begin + sz + sz2 + sz3);
  EXPECT(heap.node_count() == 1);

  auto sz5 = sz * 5;
  auto pop5 = heap.pop_off(sz5);
  EXPECT((uintptr_t)pop5 == (uintptr_t)first_begin + sz + sz2 + sz3 + sz4);
  EXPECT(heap.node_count() == 1);

  auto sz6 = sz * 6;
  auto pop6 = heap.pop_off(sz6);
  EXPECT((uintptr_t)pop6 == (uintptr_t)first_begin + sz + sz2 + sz3 + sz4 + sz5);
  EXPECT(heap.node_count() == 1);

  heap.push(pop1, pop1->size);
  heap.push(pop3, pop3->size);
  heap.push(pop5, pop5->size);
  EXPECT(heap.node_count() == 4);
  EXPECT((uintptr_t)*(heap.find_largest()) == (uintptr_t)pop6 + pop6->size);

  auto largest = heap.allocate_largest();
  EXPECT((uintptr_t)largest.ptr == (uintptr_t)pop6 + pop6->size);
  EXPECT(heap.node_count() == 3);
  EXPECT(*(heap.find_largest()) == pop5);
  EXPECT(heap.find_prior((void*)0) == nullptr);
  print_summary(heap);
  EXPECT(heap.find_prior(pop1) == nullptr);
  auto pr2 = heap.find_prior(pop2);
  EXPECT(heap.find_prior(pop2) == pop1);
}

CASE("lstack::nodes: testing lstack<merge> node traversal")
{
  using namespace util::literals;
  using Alloc = util::alloc::detail::Lstack<util::alloc::Lstack_opt::merge>;
  test::pool<Alloc, 1_MiB> pool;
  Alloc heap = pool.stack;;
  EXPECT(heap.bytes_allocated() == 0);
  EXPECT(heap.bytes_free() == pool.size);
  EXPECT(heap.node_count() == 1);
  EXPECT(heap.allocation_end() == (uintptr_t)pool.data);

  // Find largest
  auto first_begin = heap.allocation_begin();
  EXPECT(*(heap.find_largest()) == heap.allocation_begin());

  auto sz = 4_KiB;

  // Stack behavior - get the same chunk every time
  heap.deallocate(heap.allocate(sz), sz);
  heap.deallocate(heap.allocate(sz), sz);
  heap.deallocate(heap.allocate(sz), sz);
  heap.deallocate(heap.allocate(sz), sz);

  // Merging enabled
  EXPECT(heap.node_count() == 1);

  print_summary(heap);
  EXPECT((uintptr_t)(*(heap.find_largest())) == (uintptr_t)((char*)first_begin));

  auto pop1 = heap.pop_off(sz);
  EXPECT(pop1 == first_begin);
  EXPECT((uintptr_t)(*(heap.find_largest())) == (uintptr_t)((char*)first_begin + sz));
  EXPECT(heap.node_count() == 1);

  print_summary(heap);

  // Create gaps
  auto sz2 = sz * 2;
  auto pop2 = heap.pop_off(sz2);
  EXPECT((uintptr_t)pop2 == (uintptr_t)first_begin + sz);
  EXPECT(heap.node_count() == 1);

  auto sz3 = sz * 3;
  auto pop3 = heap.pop_off(sz3);
  EXPECT((uintptr_t)pop3 == (uintptr_t)first_begin + sz + sz2);
  EXPECT(heap.node_count() == 1);

  auto sz4 = sz * 4;
  auto pop4 = heap.pop_off(sz4);
  EXPECT((uintptr_t)pop4 == (uintptr_t)first_begin + sz + sz2 + sz3);
  EXPECT(heap.node_count() == 1);

  auto sz5 = sz * 5;
  auto pop5 = heap.pop_off(sz5);
  EXPECT((uintptr_t)pop5 == (uintptr_t)first_begin + sz + sz2 + sz3 + sz4);
  EXPECT(heap.node_count() == 1);

  auto sz6 = sz * 6;
  auto pop6 = heap.pop_off(sz6);
  EXPECT((uintptr_t)pop6 == (uintptr_t)first_begin + sz + sz2 + sz3 + sz4 + sz5);
  EXPECT(heap.node_count() == 1);

  heap.push(pop1, pop1->size);
  print_summary(heap);
  heap.push(pop3, pop3->size);
  heap.push(pop5, pop5->size);

  // Expect fragmentation
  EXPECT(heap.node_count() == 4);
  EXPECT((uintptr_t)*(heap.find_largest()) == (uintptr_t)pop6 + pop6->size);

  auto largest = heap.allocate_largest();
  EXPECT((uintptr_t)largest.ptr == (uintptr_t)pop6 + pop6->size);
  EXPECT((uintptr_t)largest.size != 0);
  EXPECT(heap.node_count() == 3);
  EXPECT(*(heap.find_largest()) == pop5);
  EXPECT(heap.find_prior((void*)0) == nullptr);
  print_summary(heap);
  EXPECT(heap.find_prior(pop1) == nullptr);
  auto pr2 = heap.find_prior(pop2);
  EXPECT(heap.find_prior(pop2) == pop1);

  print_summary(heap);

  // Expect perfect merging
  heap.push(pop2, sz2);
  EXPECT(heap.node_count() == 2);

  print_summary(heap);
  EXPECT_THROWS(heap.deallocate(pop3, sz3));

  heap.push(pop4, sz4);
  EXPECT(heap.node_count() == 1);

  heap.push(pop6, sz6);
  EXPECT(heap.node_count() == 1);

  heap.push(largest.ptr, largest.size);
  EXPECT(heap.node_count() == 1);
}
