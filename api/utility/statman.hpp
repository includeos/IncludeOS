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

#ifndef UTILITY_STATMAN_HPP
#define UTILITY_STATMAN_HPP

#include <cstddef>
#include <string>

// IncludeOS
#include <common>

/** Exception thrown when Statman's span is full */
class Stats_out_of_memory : public std::out_of_range {
public:
  explicit Stats_out_of_memory()
    : std::out_of_range(std::string{"Statman has no room for more statistics"})
    {}
}; //< class Stats_out_of_memory

/** Exception for Statman */
class Stats_exception : public std::runtime_error {
  using runtime_error::runtime_error;
};

class Stat {

public:
  enum stat_type
  {
    FLOAT,
    UINT32,
    UINT64
  };

  Stat(const stat_type type, const int index_into_span, const std::string& name);

  ~Stat() = default;

  void operator++();

  stat_type type() const { return type_; }

  int index() const { return index_into_span_; }

  std::string name() const { return name_; }

  float& get_float();

  uint32_t& get_uint32();

  uint64_t& get_uint64();

private:
  stat_type type_;
  int index_into_span_;
  union {
    float f;
    uint32_t ui32;
    uint64_t ui64;
  };
  char name_[48];

  Stat(const Stat& other) = delete;

  Stat(const Stat&& other) = delete;

  Stat& operator=(const Stat& other) = delete;

  Stat& operator=(Stat&& other) = delete;

};  // < class Stat

class Statman {
  /*
    @note
    This fails on g++ 5.4.0
    Passes on clang 3.8.0

    static_assert(std::is_pod<Stat>::value, "Stat is pod type");
  */

  using Span = gsl::span<Stat>;

public:
  static Statman& get() {
    static Statman statman_{0x8000, 8192};
    return statman_;
  }

  using Size_type = ptrdiff_t;
  using Span_iterator = gsl::span<Stat>::iterator;

  Statman(uintptr_t start, Size_type num_bytes);

  ~Statman() = default;

  Stat& operator[](int i) { return stats_[i]; }

  /**
   * Returns the number of elements the span stats_ can contain
   */
  Size_type size() const { return stats_.size(); }

  /**
   * Returns the number of bytes the span stats_ takes up
   */
  Size_type num_bytes() const { return num_bytes_; }

  /**
   * Returns the total number of bytes the Statman object takes up
   */
  Size_type total_num_bytes() const { return num_bytes() + sizeof(int) + sizeof(Size_type); }

  /**
   * Returns the number of Stat-objects the span stats_ actually contains
   */
  int num_stats() const { return next_available_; }

  /**
   * Is true if the span stats_ contains no Stat-objects
   */
  bool empty() const { return next_available_ == 0; }

  /**
   * Is true if the span stats_ can not contain any more Stat-objects
   */
  bool full() const { return next_available_ == stats_.size(); }

  /**
   * Returns an iterator to the last used (or filled in) element
   * in the span stats_
   */
  Span_iterator last_used();

  auto begin() { return stats_.begin(); }

  auto end() { return stats_.end(); }

  auto cbegin() const { return stats_.cbegin(); }

  auto cend() const { return stats_.cend(); }

  Stat& create(const Stat::stat_type type, const std::string& name);

private:
  Span stats_;
  int next_available_ = 0;
  Size_type num_bytes_;

  Statman(const Statman& other) = delete;

  Statman(const Statman&& other) = delete;

  Statman& operator=(const Statman& other) = delete;

  Statman& operator=(Statman&& other) = delete;

};  // < class Statman

#endif  // < UTILITY_STATMAN_HPP
