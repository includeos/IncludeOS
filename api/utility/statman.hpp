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

enum stat_type
{
  FLOAT,
  UINT32,
  UINT64
};

class Stat {

public:
  Stat(const stat_type type, const int index_into_span, const std::string& name);

  ~Stat() = default;

  Stat(const Stat& other) = delete;

  Stat(const Stat&& other) = delete;

  Stat& operator=(const Stat& other) = default;

  Stat& operator=(Stat&& other) = default;

  void operator++();

  stat_type type() const { return type_; }

  int index() const { return index_into_span_; }

  const std::string& name() const { return name_; }

  float& get_float();

  uint32_t& get_uint32();

  uint64_t& get_uint64();

  /* Option (think about solution):
     Take the time when something happens in the class (f.ex.) ++, and store it
     Then, when something happens again, check the last stored time and check the difference
     Average over time or ?
  */

private:
  stat_type type_;
  union {
    float f;
    uint32_t ui32;
    uint64_t ui64;
  };
  int index_into_span_;
  std::string name_;

};  // < class Stat

class Statman {

  using Span = gsl::span<Stat>;

public:
  using Size_type = ptrdiff_t;

  Statman(uintptr_t start, Size_type num_bytes);

  ~Statman() = default;

  Statman(const Statman& other) = delete;

  Statman(const Statman&& other) = delete;

  Statman& operator=(const Statman& other) = delete;

  Statman& operator=(Statman&& other) = delete;

  /**
   * Returns the number of elements the span stats_ can contain
   */
  Size_type size() const { return stats_.size(); }

  /**
   * Returns the number of bytes the span stats_ takes up
   */
  Size_type num_bytes() const { return num_bytes_; }

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

  uintptr_t addr_start() const noexcept {
    return reinterpret_cast<uintptr_t>(stats_.data());
  }

  uintptr_t addr_end() const noexcept {
    return reinterpret_cast<uintptr_t>(stats_.data() + stats_.size());
  }

  /**
   * Returns an iterator to the last used (or filled in) element
   * in the span stats_
   */
  auto last_used() const;

  auto begin() const { return stats_.begin(); }

  auto end() const { return stats_.end(); }

  auto cbegin() const { return stats_.cbegin(); }

  auto cend() const { return stats_.cend(); }

  Stat& create(const stat_type type, const std::string& name);

private:
  Span stats_;
  int next_available_ = 0;
  Size_type num_bytes_;

};  // < class Statman

// ----------------- Implementation details: --------------------

// class Stat:

Stat::Stat(const stat_type type, const int index_into_span, const std::string& name)
  : type_{type}, index_into_span_{index_into_span}, name_{name}
{
  switch (type) {
    case UINT32:  ui32 = 0; break;
    case UINT64:  ui64 = 0; break;
    case FLOAT:   f = 0.0f; break;
    default:      throw Stats_exception{"Creating stat: Invalid stat_type"};
  }
}

void Stat::operator++() {
  switch (type_) {
    case UINT32:  ui32++; break;
    case UINT64:  ui64++; break;
    case FLOAT:   f += 1.0f; break;
    default:      throw Stats_exception{"Incrementing stat: Invalid stat_type"};
  }
}

float& Stat::get_float() {
  if(type_ not_eq FLOAT)
    throw Stats_exception{"Get stat: stat_type is not a float"};

  return f;
}

uint32_t& Stat::get_uint32() {
  if(type_ not_eq UINT32)
    throw Stats_exception{"Get stat: stat_type is not an uint32_t"};

  return ui32;
}

uint64_t& Stat::get_uint64() {
  if(type_ not_eq UINT64)
    throw Stats_exception{"Get stat: stat_type is not an uint64_t"};

  return ui64;
}

// class Statman:

Statman::Statman(uintptr_t start, Size_type num_bytes)
{
  if(num_bytes < 0)
    throw Stats_exception{"Creating Statman: A negative number of bytes has been given"};

  Size_type num_stats_in_span = num_bytes / sizeof(Stat);

  num_bytes_ = sizeof(Stat) * num_stats_in_span;
  stats_ = Span(reinterpret_cast<Stat*>(start), num_stats_in_span);
}

auto Statman::last_used() const {
  int i = 0;

  for(auto it = stats_.begin(); it != stats_.end(); ++it) {
    if(i == next_available_)
      return it;
    i++;
  }

  return stats_.end();
}

Stat& Statman::create(const stat_type type, const std::string& name) {
  int idx = next_available_;

  if(idx >= stats_.size())
    throw Stats_out_of_memory();

  stats_[next_available_++] = Stat{type, idx, name};
  return stats_[idx];
}

#endif  // < UTILITY_STATMAN_HPP
