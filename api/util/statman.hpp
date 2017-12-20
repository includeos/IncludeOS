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
#include <membitmap>
#include <smp_utils>

struct Stats_out_of_memory : public std::out_of_range {
  explicit Stats_out_of_memory()
    : std::out_of_range("Statman has no room for more statistics")
    {}
};

struct Stats_exception : public std::runtime_error {
  using runtime_error::runtime_error;
};

class Stat {
public:
  static const int MAX_NAME_LEN = 46;

  enum Stat_type: uint8_t
  {
    FLOAT,
    UINT32,
    UINT64
  };

  Stat(const Stat_type type, const std::string& name);
  ~Stat() = default;

  // increment stat counter
  void operator++();

  ///
  Stat_type type() const noexcept
  { return type_; }

  ///
  const char* name() const noexcept
  { return name_; }

  ///
  bool unused() const noexcept {
    return name_[0] == 0;
  }

  ///
  float& get_float();

  ///
  uint32_t& get_uint32();

  ///
  uint64_t& get_uint64();

private:
  union {
    float    f;
    uint32_t ui32;
    uint64_t ui64;
  };
  Stat_type type_;

  char name_[MAX_NAME_LEN+1];

  Stat(const Stat& other) = delete;
  Stat(const Stat&& other) = delete;
  Stat& operator=(const Stat& other) = delete;
  Stat& operator=(Stat&& other) = delete;

}; //< class Stat


class Statman {
public:
  using Size_type = ptrdiff_t;

  // retrieve main instance of Statman
  static Statman& get();

  // access a Stat by a given index
  const Stat& operator[] (const int i) const
  {
    if (i < 0 || i >= cend() - cbegin())
      throw std::out_of_range("Index " + std::to_string(i) + " was out of range");
    return stats_[i];
  }
  Stat& operator[] (const int i)
  {
    return const_cast<Stat&>(static_cast<const Statman&>(*this)[i]);
  }

  /**
    * Create a new stat
   **/
  Stat& create(const Stat::Stat_type type, const std::string& name);
  // retrieve stat based on address from stats counter: &stat.get_xxx()
  Stat& get(const void* addr);
  // if you know the name of a statistic already
  Stat& get_by_name(const char* name);
  // free/delete stat based on address from stats counter
  void free(void* addr);

  /**
   * Returns the max capacity of the container
   */
  Size_type capacity() const noexcept
  { return end_stats_ - stats_; }

  /**
   * Returns the number of used elements
   */
  Size_type size() const noexcept
  { return bitmap.count_set(); }

  /**
   * Returns the number of bytes necessary to dump stats
   */
  Size_type num_bytes() const noexcept
  { return (cend() - cbegin()) * sizeof(Stat); }

  /**
   * Is true if the span stats_ contains no Stat-objects
   */
  bool empty() const noexcept
  { return size() == 0; }

  /**
   * Returns true if the container is full
   */
  bool full() const noexcept
  { return size() == capacity(); }

  /**
   *  Returns a pointer to the first element
   */
  const Stat* cbegin() const noexcept
  { return stats_; }

  /**
   *  Returns a pointer to after the last used element
   */
  const Stat* cend() const noexcept {
    int idx = bitmap.last_set() + 1; // 0 .. N-1
    return &stats_[idx];
  }

  Stat* begin() noexcept { return (Stat*) cbegin(); }
  Stat* end() noexcept { return (Stat*) cend(); }


  void init(const uintptr_t location, const Size_type size);

  Statman() {}
  Statman(const uintptr_t location, const Size_type size)
  {
    init(location, size);
  }
  ~Statman();

private:
  Stat* stats_;
  Stat* end_stats_;
  MemBitmap::word* bdata = nullptr;
  MemBitmap bitmap;
  spinlock_t stlock = 0;

  Statman(const Statman& other) = delete;
  Statman(const Statman&& other) = delete;
  Statman& operator=(const Statman& other) = delete;
  Statman& operator=(Statman&& other) = delete;
}; //< class Statman

#endif //< UTIL_STATMAN_HPP
