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

#include <net/tcp/packet.hpp>
#include <common.cxx>

using namespace std::string_literals;

extern lest::tests & specification();

CASE("round_up returns number of d-sized chunks required for n")
{
  unsigned res;
  unsigned chunk_size {128};
  res = round_up(127, chunk_size);
  EXPECT(res == 1u);
  res = round_up(128, chunk_size);
  EXPECT(res == 1u);
  res = round_up(129, chunk_size);
  EXPECT(res == 2u);
}

CASE("round_up expects div to be greater than 0")
{
  unsigned chunk_size {0};
  EXPECT_THROWS(round_up(128, chunk_size));
}
