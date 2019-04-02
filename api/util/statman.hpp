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
#include <deque>
#include <string>
#include <smp_utils>

struct Stats_out_of_memory : public std::out_of_range {
  explicit Stats_out_of_memory()
    : std::out_of_range("Statman has no room for more statistics")
    {}
};

struct Stats_exception : public std::runtime_error {
  using runtime_error::runtime_error;
};

namespace liu {
  struct Storage; struct Restore;
}

class Stat {
public:
  static const int MAX_NAME_LEN = 46;
  static const int GAUGE_BIT    = 0x40;
  static const int PERSIST_BIT  = 0x80;

  enum Stat_type: uint8_t
  {
    FLOAT,
    UINT32,
    UINT64
  };

  Stat(const Stat_type type, const std::string& name);
  Stat(const Stat& other);
  Stat& operator=(const Stat& other);
  ~Stat() = default;

  // increment stat counter
  void operator++();

  Stat_type type() const noexcept
  { return (Stat_type)(m_bits & 0xF); }

  bool is_persistent() const noexcept { return m_bits & PERSIST_BIT; }
  void make_persistent() noexcept { m_bits |= PERSIST_BIT; }

  bool is_counter() const noexcept { return (m_bits & GAUGE_BIT) == 0; }
  bool is_gauge() const noexcept { return (m_bits & GAUGE_BIT) == GAUGE_BIT; }

  void make_counter() noexcept { m_bits &= ~GAUGE_BIT; }
  void make_gauge() noexcept { m_bits |= GAUGE_BIT; }

  const char* name() const noexcept { return name_; }
  bool unused() const noexcept { return name_[0] == 0; }

  const float&    get_float() const;
  float&          get_float();
  const uint32_t& get_uint32() const;
  uint32_t&       get_uint32();
  const uint64_t& get_uint64() const;
  uint64_t&       get_uint64();

  std::string to_string() const;

private:
  union {
    float    f;
    uint32_t ui32;
    uint64_t ui64;
  };
  uint8_t m_bits;

  char name_[MAX_NAME_LEN+1];
}; //< class Stat


class Statman {
public:
  // retrieve main instance of Statman
  static Statman& get();

  // access a Stat by a given index
  const Stat& operator[] (const size_t i) const {
    return m_stats.at(i);
  }
  Stat& operator[] (const size_t i) {
    return m_stats.at(i);
  }

  /**
    * Create a new stat
   **/
  Stat& create(const Stat::Stat_type type, const std::string& name);
  // retrieve stat based on address from stats counter: &stat.get_xxx()
  Stat& get(const Stat* addr);
  // if you know the name of a statistic already
  Stat& get_by_name(const char* name);
  // free/delete stat based on address from stats counter
  void free(void* addr);

  /**
   * Returns the number of used elements
   */
  size_t size() const noexcept {
    return m_stats.size() - m_stats[0].get_uint32();
  }

  /**
   * Returns the number of bytes necessary to dump stats
   */
  size_t num_bytes() const noexcept
  { return sizeof(*this) + size() * sizeof(Stat); }

  /**
   * Is true if the span stats_ contains no Stat-objects
   */
  bool empty() const noexcept { return m_stats.empty(); }

  /* Free all stats (NB: one stat remains!) */
  void clear();

  auto begin() const noexcept { return m_stats.begin(); }
  auto end() const noexcept { return m_stats.end(); }
  auto cbegin() const noexcept { return m_stats.cbegin(); }
  auto cend() const noexcept { return m_stats.cend(); }

  void store(uint32_t id, liu::Storage&);
  void restore(liu::Restore&);

  Statman();
private:
  std::deque<Stat> m_stats;
#ifdef INCLUDEOS_SMP_ENABLE
  mutable spinlock_t stlock = 0;
#endif
  ssize_t find_free_stat() const noexcept;
  uint32_t& unused_stats();

  Statman(const Statman& other) = delete;
  Statman(const Statman&& other) = delete;
  Statman& operator=(const Statman& other) = delete;
  Statman& operator=(Statman&& other) = delete;
}; //< class Statman

inline uint32_t& Statman::unused_stats() {
  return m_stats.at(0).get_uint32();
}

inline float& Stat::get_float() {
  if (UNLIKELY(type() != FLOAT)) throw Stats_exception{"Stat type is not a float"};
  return f;
}
inline uint32_t& Stat::get_uint32() {
  if (UNLIKELY(type() != UINT32)) throw Stats_exception{"Stat type is not an uint32"};
  return ui32;
}
inline uint64_t& Stat::get_uint64() {
  if (UNLIKELY(type() != UINT64)) throw Stats_exception{"Stat type is not an uint64"};
  return ui64;
}

inline const float& Stat::get_float() const {
  if (UNLIKELY(type() != FLOAT)) throw Stats_exception{"Stat type is not a float"};
  return f;
}
inline const uint32_t& Stat::get_uint32() const {
  if (UNLIKELY(type() != UINT32)) throw Stats_exception{"Stat type is not an uint32"};
  return ui32;
}
inline const uint64_t& Stat::get_uint64() const {
  if (UNLIKELY(type() != UINT64)) throw Stats_exception{"Stat type is not an uint64"};
  return ui64;
}

#endif //< UTIL_STATMAN_HPP
