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

#pragma once
#ifndef UTIL_FIXED_BITMAP_HPP
#define UTIL_FIXED_BITMAP_HPP

#include "membitmap.hpp"
#include <array>

/**
 * @brief      A membitmap with a fixed amount of bits and storage.
 *
 * @tparam     N     Number of bits. Needs to be divisable by sizeof(Storage)
 */
template <size_t N>
class Fixed_bitmap : public MemBitmap {
public:
  using Storage = MemBitmap::word;
  static_assert(N >= sizeof(Storage), "Number of bits need to be atleast sizeof(Storage)");
  static_assert(N % sizeof(Storage) == 0, "Number of bits need to be divisable by sizeof(Storage)");

public:
  Fixed_bitmap() :
    MemBitmap{},
    storage{}
  {
    set_location(storage.data(), N / sizeof(Storage));
  }

private:
  std::array<Storage, N / sizeof(Storage)> storage;

}; // < class Fixed_bitmap

#endif
