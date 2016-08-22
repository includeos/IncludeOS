// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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
#ifndef UTIL_FIXEDVEC_HPP
#define UTIL_FIXEDVEC_HPP

/**
 * High performance no-heap fixed vector
 * 
 **/

#include <cstdint>
#include <cstring>

template <typename T, int N>
struct fixedvector {
  
  void add(const T& e) noexcept {
    element[count] = e;
    count++;
  }
  
  void clear() noexcept {
    count = 0;
  }
  uint32_t size() const noexcept {
    return count;
  }
  
  T* first() noexcept {
    return &element[0];
  }
  T* end() noexcept {
    return &element[count];
  }
  
  bool free_capacity() const noexcept {
    return count < N;
  }
  
  void clone(T* src, uint32_t size) {
    memcpy(element, src, size * sizeof(T));
    count = size;
  }
  
  uint32_t count = 0;
  T element[N];
};


#endif
