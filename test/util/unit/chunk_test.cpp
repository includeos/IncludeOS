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

static const size_t N = 1024;

CASE("Constructing an empty chunk")
{
  Chunk c;

  EXPECT(c.length() == 0);

  EXPECT_THROWS_AS(c.at(0), std::out_of_range);

  EXPECT_NOT(c);
}
CASE("Constructing a chunk with a shared buffer and size")
{
  auto buf = Chunk::make_shared_buffer(N);

  Chunk c{buf, N};

  EXPECT(c.size() == N);
  EXPECT(c);

  EXPECT(c.data() == buf.get());
}
CASE("Constructing a chunk with size")
{
  Chunk c{N};

  EXPECT(c.size() == N);
  EXPECT(c);
}
CASE("Constructing a chunk with data and size")
{
  const uint8_t* data = new uint8_t[N]{1};

  Chunk c{data, N};

  EXPECT(c.size() == N);

  auto res = std::memcmp(data, c.data(), N);

  EXPECT(res == 0);
}
CASE("Copy and move construction and assignment")
{
  const Chunk base{10};
  std::memset(base.data(), 'A', base.size());

  Chunk c1{base};
  EXPECT(c1 == base);

  Chunk c2{std::move(c1)};
  EXPECT(c2 == base);

  Chunk c3;
  c3 = base;
  EXPECT(c3 == base);

  Chunk c4;
  c4 = std::move(c3);
  EXPECT(c4 == base);
}
CASE("Combining two chunks into a new chunk")
{
  const size_t SZ = 10;

  Chunk c1{SZ};
  std::memset(c1.data(), 'A', c1.size());

  Chunk c2{SZ};
  std::memset(c2.data(), 'B', c2.size());

  EXPECT_NOT(c1 == c2);
  EXPECT(c1 != c2);

  auto c3 = c1 + c2;
  EXPECT(c3.size() == (c1.size() + c2.size()));

  auto r1 = std::memcmp(c3.data(), c1.data(), c1.size());
  auto r2 = std::memcmp(c3.data() + c1.size(), c2.data(), c2.size());

  EXPECT((r1 + r2) == 0);
}
CASE("Operations and iterators")
{
  const std::string test{"This string is for testing chunks."};

  Chunk c{(const uint8_t*) test.data(), test.size()};

  EXPECT(c.size() == test.size());

  EXPECT(std::memcmp(c.data(), test.data(), c.size()) == 0);

  EXPECT(c[6] == test[6]);

  EXPECT_THROWS_AS(c.at(test.size()+1), std::out_of_range);

  std::string test2{c.begin(), c.end()};

  EXPECT(test2 == test);

  std::string test3;
  for(auto it = c.begin(); it != c.end(); ++it)
    test3.push_back((const char)(*it));

  EXPECT(test3 == test);

  c.at(c.size() - 1) = '!';

  EXPECT_NOT(std::memcmp(c.data(), test.data(), c.size()) == 0);

  const std::string test4{"This string is for testing chunks!"};

  EXPECT(std::memcmp(c.data(), test4.data(), c.size()) == 0);

  c.clear();

  EXPECT_NOT(std::memcmp(c.data(), test.data(), c.size()) == 0);

  auto is_cleared = [&c]()->bool
  {
    for(auto&& byte : c)
      if(byte != 0) return false;
    return true;
  };
  EXPECT(is_cleared());
}
CASE("Filling chunks")
{
  const std::string test{"This string is for testing chunks."};

  Chunk c1{test.size()};
  auto written = c1.fill((const uint8_t*) test.data(), test.size());

  EXPECT(written == test.size());

  EXPECT(std::memcmp(c1.data(), test.data(), c1.size()) == 0);

  const auto half = test.size() / 2;

  written = c1.fill((const uint8_t*) test.data(), test.size(), half);

  EXPECT(written == half);

  EXPECT_NOT(std::memcmp(c1.data(), test.data(), c1.size()) == 0);

  EXPECT(std::memcmp(c1.data() + half, test.data(), half) == 0);

  Chunk c2{half};

  written = c2.fill((const uint8_t*) test.data(), test.size());

  EXPECT(written == half);

  std::string test2{c2.begin(), c2.end()};
  EXPECT(test2 == test.substr(0, half));

  written = c2.fill((const uint8_t*) test.data(), test.size(), half);
  EXPECT(written == 0);
}
