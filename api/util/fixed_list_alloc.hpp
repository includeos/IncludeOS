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
#ifndef UTIL_FIXED_LIST_ALLOC_HPP
#define UTIL_FIXED_LIST_ALLOC_HPP

#include "fixed_storage.hpp"

// Implemenation based upon Howard Hinnant's terrific short_alloc example
// https://howardhinnant.github.io/short_alloc.h

/**
 * @brief      A fixed list allocator. Keeps its own fixed storage.
 *
 * @note       Only tested together with list,
 *             should not be used with other containers.
 *
 * @tparam     T      The type of object to be allocated
 * @tparam     N      The maximum number of objects in storage
 * @tparam     Align  The alignment of the object
 */
template <class T, std::size_t N, std::size_t Align = alignof(std::ptrdiff_t)>
class Fixed_list_alloc {
public:
  using value_type = T;
  static auto constexpr alignment = Align;
  static auto constexpr size = N;
  using storage_type = Fixed_storage<T, size, alignment>;

  static_assert((sizeof(T) * size) % alignment == 0,
    "Total size (sizeof(T) * N) needs to be a multiple of alignment Align");

private:
    storage_type store_;

public:
  Fixed_list_alloc() noexcept
    : store_() {}

  Fixed_list_alloc(const Fixed_list_alloc&) = delete;
  Fixed_list_alloc& operator=(const Fixed_list_alloc&) = delete;

  storage_type& storage() noexcept
  { return store_; }

  template <class _Up> struct rebind {
    using other = Fixed_list_alloc<_Up, N, alignment>;
  };

  T* allocate(std::size_t n)
  {
    return reinterpret_cast<T*>(store_.template allocate<alignof(T)>(n*sizeof(T)));
  }

  void deallocate(T* p, std::size_t n) noexcept
  {
    store_.deallocate(reinterpret_cast<char*>(p), n*sizeof(T));
  }

  bool operator==(const Fixed_list_alloc& other) const noexcept
  { return this == &other; }

  bool operator!=(const Fixed_list_alloc& other) const noexcept
  { return !(*this == other); }

  template <class T1, std::size_t N1, std::size_t A1,
            class U, std::size_t M, std::size_t A2>
  friend bool operator==(const Fixed_list_alloc<T1, N1, A1>& x,
                         const Fixed_list_alloc<U, M, A2>& y) noexcept;

  template <class U, std::size_t M, std::size_t A> friend class Fixed_list_alloc;

}; // < class Fixed_list_alloc

template <class T, std::size_t N, std::size_t A1,
          class U, std::size_t M, std::size_t A2>
inline bool operator==(const Fixed_list_alloc<T, N, A1>& x,
                       const Fixed_list_alloc<U, M, A2>& y) noexcept
{
  return false; // prevent deallocating eachothers resources
}

template <class T, std::size_t N, std::size_t A1,
          class U, std::size_t M, std::size_t A2>
inline bool operator!=(const Fixed_list_alloc<T, N, A1>& x,
                       const Fixed_list_alloc<U, M, A2>& y) noexcept
{
  return !(x == y);
}

#endif
