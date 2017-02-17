// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef UTIL_STATMAN_HPP
#define UTIL_STATMAN_HPP

#include <common>
#include <cstddef>
#include <string>

///
/// This type is thrown when Statman's span is full
///
struct Stats_out_of_memory : public std::out_of_range {
  explicit Stats_out_of_memory()
    : std::out_of_range(std::string{"Statman has no room for more statistics"})
    {}
}; //< struct Stats_out_of_memory

///
/// This type is thrown from within the operations of class Statman
///
struct Stats_exception : public std::runtime_error {
  using runtime_error::runtime_error;
}; //< struct Stats_exception

///
///
///
class Stat {
private:
  constexpr static int MAX_NAME_LEN {48};
public:
  ///
  ///
  ///
  enum Stat_type
  {
    FLOAT,
    UINT32,
    UINT64
  };

  ///
  ///
  ///
  Stat(const Stat_type type, const int index_into_span, const std::string& name);

  ///
  ///
  ///
  ~Stat() = default;

  ///
  ///
  ///
  void operator++();

  ///
  ///
  ///
  Stat_type type() const noexcept
  { return type_; }

  ///
  ///
  ///
  int index() const noexcept
  { return index_into_span_; }

  ///
  ///
  ///
  const char* name() const noexcept
  { return name_; }

  ///
  ///
  ///
  float& get_float();

  ///
  ///
  ///
  uint32_t& get_uint32();

  ///
  ///
  ///
  uint64_t& get_uint64();

private:
  Stat_type type_;
  int       index_into_span_;

  union {
    float    f;
    uint32_t ui32;
    uint64_t ui64;
  };

  char name_[MAX_NAME_LEN];

  Stat(const Stat& other) = delete;
  Stat(const Stat&& other) = delete;
  Stat& operator=(const Stat& other) = delete;
  Stat& operator=(Stat&& other) = delete;
}; //< class Stat

///
///
///
class Statman {
  /*
    @note
    This fails on g++ 5.4.0
    Passes on clang 3.8.0

    static_assert(std::is_pod<Stat>::value, "Stat is pod type");
  */

  using Span = gsl::span<Stat>;
public:
  using Size_type     = ptrdiff_t;
  using Span_iterator = Span::iterator;

  ///
  ///
  ///
  static Statman& get();

  ///
  ///
  ///
  Statman(const uintptr_t start, const Size_type num_bytes);

  ///
  ///
  ///
  ~Statman() = default;

  ///
  ///
  ///
  Stat& operator[](const int i)
  { return stats_[i]; }

  /**
   * Returns the number of elements the span stats_ can contain
   */
  Size_type size() const noexcept
  { return stats_.size(); }

  /**
   * Returns the number of bytes the span stats_ takes up
   */
  Size_type num_bytes() const noexcept
  { return num_bytes_; }

  /**
   * Returns the total number of bytes the Statman object takes up
   */
  Size_type total_num_bytes() const noexcept
  { return num_bytes() + sizeof(int) + sizeof(Size_type); }

  /**
   * Returns the number of Stat-objects the span stats_ actually contains
   */
  int num_stats() const noexcept
  { return next_available_; }

  /**
   * Is true if the span stats_ contains no Stat-objects
   */
  bool empty() const noexcept
  { return next_available_ == 0; }

  /**
   * Is true if the span stats_ can not contain any more Stat-objects
   */
  bool full() const noexcept
  { return next_available_ == stats_.size(); }

  /**
   * Returns an iterator to the last used (or filled in) element
   * in the span stats_
   */
  Span_iterator last_used();

  ///
  ///
  ///
  auto begin() noexcept
  { return stats_.begin(); }

  ///
  ///
  ///
  auto end() noexcept
  { return stats_.end(); }

  ///
  ///
  ///
  auto cbegin() const noexcept
  { return stats_.cbegin(); }

  ///
  ///
  ///
  auto cend() const noexcept
  { return stats_.cend(); }

  ///
  ///
  ///
  Stat& create(const Stat::Stat_type type, const std::string& name);

  ///
  ///
  ///
  Stat& get(const std::string& name);
private:
  Span      stats_;
  int       next_available_ {0};
  Size_type num_bytes_;

  Statman(const Statman& other) = delete;
  Statman(const Statman&& other) = delete;
  Statman& operator=(const Statman& other) = delete;
  Statman& operator=(Statman&& other) = delete;
}; //< class Statman

#endif //< UTIL_STATMAN_HPP
