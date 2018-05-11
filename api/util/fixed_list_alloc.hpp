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
#include <memory>

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
  using propagate_on_container_move_assignment = std::true_type;
private:
    std::unique_ptr<storage_type> store_;

public:
  Fixed_list_alloc()
    : store_{nullptr}
  {}

  template<typename U>
  Fixed_list_alloc(const U&)
    : store_{nullptr}
  {
    // ain't much to do here since storage is fixed to T,
    // so we just create a new storage for this type
  }

  Fixed_list_alloc(Fixed_list_alloc&&) noexcept = default;
  Fixed_list_alloc& operator=(Fixed_list_alloc&&) noexcept = default;

  // I'd really like to disable copy construction, since it makes no sense here.
  // >=Clang5 and >=GCC8 allows this ('rebind' resp. 'move constuctor' being used),
  // but GCC7 does not (unecessary utilize copy constructor).
  Fixed_list_alloc(const Fixed_list_alloc&)
    : store_{nullptr}
  {
    // we can't really copy due to fixed_storage,
    // just create new storage and hope no assumptions
    // are made that the allocators are the same
  }

  /* No copy */
  //Fixed_list_alloc(const Fixed_list_alloc&) = delete;
  Fixed_list_alloc& operator=(const Fixed_list_alloc&) = delete;

  static Fixed_list_alloc select_on_container_copy_construction(const Fixed_list_alloc&)
  {
    return {};
  }

  template<typename U>
  using other = Fixed_list_alloc<U, N, alignment>;

  template <class U> struct rebind {
    using other = Fixed_list_alloc<U, N, alignment>;
  };

  T* allocate(std::size_t n)
  {
    if(UNLIKELY(store_ == nullptr))
      store_.reset(new storage_type);
    return reinterpret_cast<T*>(store_->template allocate<alignof(T)>(n*sizeof(T)));
  }

  void deallocate(T* p, std::size_t n) noexcept
  {
    store_->deallocate(reinterpret_cast<char*>(p), n*sizeof(T));
  }

  bool operator==(const Fixed_list_alloc& other) const noexcept
  { return this->store_ == other.store_; }

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
  return x == y;
}

template <class T, std::size_t N, std::size_t A1,
          class U, std::size_t M, std::size_t A2>
inline bool operator!=(const Fixed_list_alloc<T, N, A1>& x,
                       const Fixed_list_alloc<U, M, A2>& y) noexcept
{
  return !(x == y);
}

#endif
