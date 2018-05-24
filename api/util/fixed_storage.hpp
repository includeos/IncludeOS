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
#ifndef UTIL_FIXED_STORAGE_HPP
#define UTIL_FIXED_STORAGE_HPP

#include <array>
#include <common>
#include <util/fixed_vector.hpp>

// Implemenation based upon Howard Hinnant's terrific short_alloc example
// https://howardhinnant.github.io/short_alloc.h

struct Fixed_storage_error : public std::runtime_error {
  using runtime_error::runtime_error;
};

/**
 * @brief      Fixed aligned storage (to be used in allocator).
 *             Pre-allocates enough storage for the number of aligned objects.
 *             Takes a object type, but underlying storage is just a char array.
 *
 * @tparam     T          What type of object to be stored
 * @tparam     N          Number of objects that can be stored
 * @tparam     alignment  Pointer alignment
 */
template <typename T, std::size_t N, std::size_t alignment = alignof(std::ptrdiff_t)>
class Fixed_storage {
public:
  static constexpr std::size_t aligned_size() noexcept
  { return (sizeof(T) + (alignment-1)) & ~(alignment-1); }

  static constexpr std::size_t buffer_size() noexcept
  { return N * aligned_size(); }

private:
  alignas(alignment) std::array<char, buffer_size()> buf_;
  /** Available addresses */
  Fixed_vector<char*, N> free_;

public:
  Fixed_storage() noexcept;

  Fixed_storage(const Fixed_storage&) = delete;
  Fixed_storage(Fixed_storage&&) = delete;
  Fixed_storage& operator=(const Fixed_storage&) = delete;
  Fixed_storage& operator=(Fixed_storage&&) = delete;

  template <std::size_t ReqAlign> char* allocate(std::size_t n);
  void deallocate(char* p, std::size_t n) noexcept;

  static constexpr std::size_t size() noexcept
  { return N; }

  std::size_t used() const noexcept
  { return buffer_size() - available(); }

  std::size_t available() const noexcept
  { return (free_.size() * aligned_size()); }

  void reset() noexcept;

private:
  constexpr const char* end() const noexcept
  { return &buf_[buffer_size()]; }

  bool pointer_in_buffer(char* p) const noexcept
  { return buf_.data() <= p && p <= end(); }

}; // < class Fixed_storage

template <typename T, std::size_t N, std::size_t alignment>
Fixed_storage<T, N, alignment>::Fixed_storage() noexcept
{
  //printf("Fixed_storage<%s, %u, %u> aligned_size: %u\n",
  //  typeid(T).name(), N, alignment, aligned_size());
  reset();
}

template <typename T, std::size_t N, std::size_t alignment>
template <std::size_t ReqAlign>
char* Fixed_storage<T, N, alignment>::allocate(std::size_t n)
{
  static_assert(ReqAlign <= alignment,
    "Alignment is too small for this storage");

  Expects(n == sizeof(T) &&
    "Allocating more than sizeof(T) not supported");

  if(UNLIKELY(free_.empty()))
    throw Fixed_storage_error{
      "Fixed_storage_error: Storage exhausted (all addresses in use)"};

  return free_.pop_back();
}

template <typename T, std::size_t N, std::size_t alignment>
void Fixed_storage<T, N, alignment>::deallocate(char* p, std::size_t) noexcept
{
  //printf("Fixed_storage<%s, %u, %u> dealloc %p\n",
  //  typeid(T).name(), N, alignment, p);

  Expects(pointer_in_buffer(p) &&
    "Trying to deallocate pointer outside my buffer");

  free_.push_back(p);

  Ensures(free_.size() <= N);
}

template <typename T, std::size_t N, std::size_t alignment>
void Fixed_storage<T, N, alignment>::reset() noexcept
{
  auto* ptr = &buf_[N * aligned_size()];
  while(ptr > buf_.data())
  {
    ptr -= aligned_size();
    free_.push_back(ptr);
  }

  Ensures(free_.size() == N);
}

#endif
