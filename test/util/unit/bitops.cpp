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
#include <util/bitops.hpp>
#include <cmath>

enum class Flags : uint8_t {
  none = 0x0,
  a = 0x1,
  b = 0x2,
  ab = 0x3,
    c = 0x4,
  d = 0x8,
  e = 0x10,
  f = 0x20,
  g = 0x40,
  h = 0x80,
  all = a | b | c | d | e | f | g | h
};

using namespace util;

// Enable bitmask ops for the Flags enum
template<>
struct bitops::enable_bitmask_ops<Flags> {
  using type = std::underlying_type<Flags>::type;
  static constexpr bool enable = true;
};

CASE ("util::bitops: Using bitmask ops for an enum")
{

  Flags f {};
  EXPECT(f == Flags::none);

  // and, or, not, xor
  EXPECT((f & Flags::a) == Flags::none);
  EXPECT((f | Flags::a | Flags::b) == Flags::ab);
  EXPECT(~f == Flags::all);
  EXPECT((f ^ Flags::c) == Flags::c);

  // Compound assignment
  f |= Flags::e;
  EXPECT(f == Flags::e);
  EXPECT((uint8_t)f == 0x10);

  f &= Flags::d;
  EXPECT(f == Flags::none);

  f |= Flags::c | Flags::h;
  EXPECT(f == (Flags::c | Flags::h));
  EXPECT((uint8_t)f == 0x84);

  f ^= Flags::c;
  EXPECT(f == Flags::h);

  // boolean flag tests
  f |= Flags::c | Flags::h;
  EXPECT(has_flag(f, Flags::c));
  EXPECT(has_flag(f, Flags::h));
  EXPECT(has_flag(f, Flags::h | Flags::c));
  EXPECT(not has_flag(f, Flags::a));
  EXPECT(not has_flag(f, Flags::all));

}

// Enable bitmask ops for int, to use in combination with other enabled types
template<>
struct bitops::enable_bitmask_ops<int> {
  using type = int;
  static constexpr bool enable = true;
};


CASE ("util::bitops: Using bitmask ops an enum and an integral")
{

  Flags f { Flags::e };
  EXPECT((f & 0x10) == Flags::e);
  EXPECT((f | 0x20) == (Flags::e | Flags::f));
  EXPECT((f ^ 0x20) == (Flags::e | Flags::f));

  EXPECT((0x10 & f) == 0x10);
  EXPECT((0x20 | f) == (0x10 | 0x20));
  EXPECT((0x20 ^ f) == (0x10 | 0x20));

}


CASE ("util::bitops: using various bit operations")
{
  EXPECT(__builtin_clzl(0x1000) == 51);

  EXPECT(bits::keeplast(0x10110) == 0x10000);
  EXPECT(bits::keeplast(0x1010001000010000) == 0x1000000000000000);
  EXPECT(bits::keeplast(0x2010001000010000) == 0x2000000000000000);
  EXPECT(bits::keeplast(0x8010001000010000) == 0x8000000000000000);
  EXPECT(bits::keeplast(0x69a8e2d82a387f69) == 0x4000000000000000);

  EXPECT(bits::keepfirst(0x8010001000010000) == 0x10000);
  EXPECT(bits::keepfirst(0x10110) == 0x10);
  EXPECT(bits::keepfirst(0x1010001000010000) == 0x10000);
  EXPECT(bits::keepfirst(0x2010001000010000) == 0x10000);
  EXPECT(bits::keepfirst(0x8010001000010000) == 0x10000);
  EXPECT(bits::keepfirst(0x69a8e2d82a387f69) == 0x1);

  EXPECT(bits::popcount(0x8010001000010000) == 4);

  for (auto i : test::random){
    EXPECT(bits::keeplast(i) == (1LLU << (uintptr_t)log2(i)));
    EXPECT(bits::keeplast(i) == (uintptr_t)(pow(2,(uintptr_t)log2(i))));
    EXPECT(bits::fls(i) - 1 == (uintptr_t)log2(i));
    EXPECT(bits::keepfirst(i) == (1LLU << (__builtin_ffs(i) - 1)));
    EXPECT(bits::popcount(i) == __builtin_popcountl(i));
  }
}
