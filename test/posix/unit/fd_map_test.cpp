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

#include <common.cxx>
#include <include/fd.hpp>
#include <include/fd_map.hpp>

class Test_fd : public FD {
public:
  Test_fd(const int id) : FD(id) {};

  int read(void*, size_t) override
  { return 1; }

  int close() override
  { return 0; }
};

class Hest_fd : public FD {
public:
  Hest_fd(const int id, std::string sound)
    : FD(id), sound_(std::move(sound))
  {}

  int close() override
  { return 0; }
private:
  std::string sound_;
};

CASE("Adding a implemented FD descriptor in FD_map")
{
  // Create
  auto& test = FD_map::_open<Test_fd>();

  // Unique ID
  auto& test2 = FD_map::_open<Test_fd>();
  EXPECT_NOT(test == test2);

  // Overriden function works
  const auto res = test.read(nullptr, 0);
  EXPECT(res == 1);

  // Get works
  const FD_map::id_t id = test.get_id();
  auto& get = FD_map::_get(id);
  EXPECT(get == test);

  // Close works
  FD_map::_close(id);

  // Throws when not found works
  EXPECT_THROWS_AS(FD_map::_get(id), FD_not_found);

  const int bet = 322; // this is a throw
  EXPECT_THROWS_AS(FD_map::_get(bet), FD_not_found);
}
