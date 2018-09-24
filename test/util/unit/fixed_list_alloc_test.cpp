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

struct Block {
  size_t id;

  Block(size_t i)
    : id{i} {}

  bool operator==(const Block& other) const
  { return id == other.id; }
};

template <std::size_t N>
using Block_list = std::list<Block, Fixed_list_alloc<Block, N>>;

CASE("Using Fixed_list_alloc")
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
