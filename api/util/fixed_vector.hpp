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
#ifndef UTIL_FIXED_VECTOR_HPP
#define UTIL_FIXED_VECTOR_HPP

/**
 * High performance no-heap fixed vector
 * All data is reserved uninitialized
 **/

#include <cstdint>
#include <cstring>
#include <cassert>
#include <type_traits>
#include <iterator>

enum class Fixedvector_Init {
  UNINIT
};

template <typename T, int N>
class Fixed_vector {
public:
  using value_type = T;
  using iterator = T*;

  Fixed_vector() : count(0) {}
  Fixed_vector(Fixedvector_Init) {}

  Fixed_vector(std::initializer_list<T> l)
    : count(l.size())
  {
    Expects(count <= capacity());
    std::memcpy(begin(), l.begin(), count * sizeof(T));
  }

  // add existing
  T& push_back(const T& e) noexcept {
    assert(count < N);
    (*this)[count] = e;
    return (*this)[count++];
  }
  // construct into
  template <typename... Args>
  T& emplace_back(Args&&... args) noexcept {
    assert(count < N);
    new (&element[count]) T(args...);
    return (*this)[count++];
  }

  /**
   * @brief      Insert a range in the vector, replacing content if pos != end
   *
   * @param[in]  pos      The position
   * @param[in]  first    The first
   * @param[in]  last     The last
   *
   * @tparam     InputIt  { description }
   */
  template <class InputIt>
  void insert_replace(iterator pos, InputIt first, InputIt last)
  {
    assert(begin() <= pos and pos <= end() && "pos do not belong to this vector");
    auto len = std::distance(first, last);
    assert((pos + len) <= (end() + remaining()) && "not enough room in vector");

    // update count
    if(pos + len >= end())
      count += (len - (end() - pos));

    while(first != last)
      *(pos++) = *(first++);
  }

  // pop back and return last element
  T pop_back() {
    return (*this)[--count];
  }
  // clear whole thing
  void clear() noexcept {
    count = 0;
  }

  bool empty() const noexcept {
    return count == 0;
  }
  uint32_t size() const noexcept {
    return count;
  }

  uint32_t remaining() const noexcept
  { return capacity() - size(); }

  T& operator[] (uint32_t i) noexcept {
    return *(T*) (element + i);
  }
  T* at (uint32_t i) noexcept {
    if (i >= size()) return nullptr;
    return (T*) (element + i);
  }

  T* data() noexcept {
    return (T*) &element[0];
  }
  T* begin() noexcept {
    return (T*) &element[0];
  }
  T* end() noexcept {
    return (T*) &element[count];
  }

  const T* data() const noexcept {
    return (T*) &element[0];
  }
  const T* begin() const noexcept {
    return (T*) &element[0];
  }
  const T* end() const noexcept {
    return (T*) &element[count];
  }

  T& back() noexcept {
    assert(not empty());
    return (T&)element[count-1];
  }

  constexpr int capacity() const noexcept {
    return N;
  }
  bool free_capacity() const noexcept {
    return count < N;
  }

  // overwrite this element buffer with a buffer from another
  // source of the same type T, with @size elements
  // Note: size and capacity are not related, and they don't have to match
  void copy(T* src, uint32_t size) {
    memcpy(element, src, size * sizeof(T));
    count = size;
  }

  template <typename R>
  bool operator==(const R& rhs) const noexcept
  {
    return size() == rhs.size() and std::memcmp(data(), rhs.data(), size()*sizeof(T)) == 0;
  }

private:
  uint32_t count;
  typename std::aligned_storage<sizeof(T), alignof(T)>::type element[N];
};


#endif
