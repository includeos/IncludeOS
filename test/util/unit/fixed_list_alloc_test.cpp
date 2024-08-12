// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#include <common.cxx>
#include <util/fixed_list_alloc.hpp>
#include <list>

// Test cases for the fixed storage backend for Fixed_alloc
// Note: this should be moved to a separate test if Fixed_storage gets used by
//       anything other than Fixed_list_alloc.
CASE("Using Fixed_storage directly for a small buffer")
{
  Fixed_storage<int, 5, alignof(int)> some_ints;

  // Verify initial state
  EXPECT(some_ints.buffer_size() == 5 * alignof(int));
  EXPECT(some_ints.aligned_size() == alignof(int));
  EXPECT(some_ints.used() == 0);
  EXPECT(some_ints.available() == some_ints.buffer_size());

  // Test invalid allocations
  EXPECT_THROWS(some_ints.allocate<alignof(int)>(6));
  EXPECT_THROWS(some_ints.allocate<alignof(int)>(0));

  // Test a valid allocation
  char* int1 = some_ints.allocate<alignof(int)>(sizeof(int));
  EXPECT(int1 != nullptr);

  // Validate resulting state
  EXPECT(some_ints.used() == sizeof(int));
  EXPECT(some_ints.available() == some_ints.buffer_size() - sizeof(int));

  // Test allocating the remainder
  char* int2 = some_ints.allocate<alignof(int)>(sizeof(int));
  char* int3 = some_ints.allocate<alignof(int)>(sizeof(int));
  char* int4 = some_ints.allocate<alignof(int)>(sizeof(int));
  char* int5 = some_ints.allocate<alignof(int)>(sizeof(int));

  EXPECT(int2 != nullptr);
  EXPECT(int3 != nullptr);
  EXPECT(int4 != nullptr);
  EXPECT(int5 != nullptr);
  EXPECT(some_ints.used() == 5 * sizeof(int));

  // Test allocating when depleted
  EXPECT_THROWS(some_ints.allocate<alignof(int)>(sizeof(int)));

  // Test deallocation
  some_ints.deallocate(int1, sizeof(int));
  EXPECT(some_ints.used() == 4 * sizeof(int));

  some_ints.deallocate(int2, sizeof(int));
  EXPECT(some_ints.used() == 3 * sizeof(int));

  some_ints.deallocate(int3, sizeof(int));
  EXPECT(some_ints.used() == 2 * sizeof(int));

  some_ints.deallocate(int4, sizeof(int));
  EXPECT(some_ints.used() == 1 * sizeof(int));

  some_ints.deallocate(int5, sizeof(int));
  EXPECT(some_ints.used() == 0);
  EXPECT(some_ints.available() == some_ints.buffer_size());
}

struct Block {
  size_t id;

  Block(size_t i)
    : id{i} {}

  bool operator==(const Block& other) const
  { return id == other.id; }
};

template <std::size_t N>
using Block_list = std::list<Block, Fixed_list_alloc<Block, N>>;


CASE("Using Fixed_list_alloc directly")
{
  const int N = 10;//'000;
  Fixed_list_alloc<int, N> alloc;

  std::vector<int*> ints;

  for (int i = 0; i < N; i++){
    int* int1 = alloc.allocate(1);
    EXPECT(int1 != nullptr);
    EXPECT(std::find(ints.begin(), ints.end(), int1) == ints.end());
    ints.emplace_back(int1);
  }

  EXPECT_THROWS(alloc.allocate(1));

  for (int* i : ints) {
    alloc.deallocate(i, 1);
  }

  int* int1 = alloc.allocate(1);
  EXPECT(int1 != nullptr);
}

CASE("Using Fixed_list_alloc with a small buffer")
{
  const int N = 10;
  Block_list<N> list{};

  EXPECT(list.empty());

  Block& first = list.emplace_back(42);

  EXPECT(! list.empty());

  Block& second = list.emplace_back(43);

  EXPECT(&first != &second);
  EXPECT(! list.empty());

  // Testing destruction
  auto listptr = std::make_unique<Block_list<N>>();
  EXPECT(listptr != nullptr);

  // Delete an empty list
  delete listptr.release();

  EXPECT(listptr == nullptr);
  listptr = std::make_unique<Block_list<N>>();
  listptr->push_back({42});

  EXPECT(! listptr->empty());
  EXPECT(listptr->front().id == 42);
  EXPECT(listptr->back().id == 42);

  listptr->push_back({43});

  EXPECT(! listptr->empty());
  EXPECT(listptr->front().id == 42);
  EXPECT(listptr->back().id == 43);

  // Delete a non-empty list
  delete listptr.release();
  EXPECT(listptr == nullptr);
}

CASE("Using Fixed_list_alloc with a large buffer")
{
  const int N = 10'000;
  Block_list<N> list{{0},{1},{2}};

  for(int i = 3; i < N; i++)
    list.emplace_back(i);

  EXPECT(list.size() == N);

  for(int i = 0; i < N/2; i++)
    list.pop_front();

  EXPECT(list.size() == N/2);

  for(int i = 0; i < N/2; i++)
    list.emplace_back(i);

  EXPECT_THROWS_AS(list.emplace_front(322), Fixed_storage_error);

  EXPECT(list.size() == N);

  auto it = std::find(list.begin(), list.end(), Block{3});
  list.splice(list.begin(), list, it);

  EXPECT(list.begin() == it);

  for(int i = 0; i < N; i++)
    list.pop_back();

  EXPECT(list.empty());

  for(auto& node : list)
    (void) node;
}
