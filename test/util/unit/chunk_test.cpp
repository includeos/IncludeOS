// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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
#include <util/chunk.hpp>

CASE("Testing a chunk")
{
  Chunk c1{1500};

  std::memset(c1.data(), 'A', 1500);

  Chunk c2{1500};

  std::memset(c2.data(), 'B', 1500);

  auto c3 = c1 + c2;

  //auto it = c.begin();

  std::string test{c3.begin(), c3.end()};

  printf("Test:%s\n", test.c_str());

  for(auto byte : c1)
    printf("%c", byte);

  EXPECT(c1[100] == 'A');
}
