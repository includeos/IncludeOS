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
#ifndef KERNEL_MEMMAP_HPP
#define KERNEL_MEMMAP_HPP

#include <cassert>
#include <common>
#include <delegate>
#include <map>
#include <string>

namespace os::mem {

/**
 * This type is used to represent an error that occurred
 * from within the operations of class Fixed_memory_range
 */
struct Memory_range_exception : public std::runtime_error {
  using runtime_error::runtime_error;
}; //< struct Memory_range_exception

/**
 * This class is used to represent a fixed and occupied
 * memory range
 */
class Fixed_memory_range {
public:
  using size_type    = ptrdiff_t;
  using In_use_delg  = delegate<size_type()>;
  using Memory_range = gsl::span<uint8_t>;

  /**
   * Constructor
   *
   * @param begin
   *   The beginning address of the memory range
   *
   * @param end
   *   The end address of the memory range
   *
   * @param name
   *   The name of the memory range
   *
   * @param in_use_operation
   *   The operation to perform when bytes_in_use is called
   *
   * @throws Memory_range_exception
   *   IIf (begin > end) or ((end - begin + 1) > max_size())
   */
  Fixed_memory_range(const uintptr_t begin, const uintptr_t end, const char* name,
                     In_use_delg in_use_operation);

  /**
   * Constructor
   *
   * @param begin
   *   The beginning address of the memory range
   *
   * @param end
   *   The end address of the memory range
   *
   * @param name
   *   The name of the memory range
   *
   * @throws Memory_range_exception
   *   IIf (begin > end) or ((end - begin + 1) > max_size())
   */
  Fixed_memory_range(const uintptr_t begin, const uintptr_t end, const char* name)
    : Fixed_memory_range(begin, end, name, nullptr)
  {}

  /**
   * Constructor
   *
   * @param range
   *   The memory range
   *
   * @param name
   *   The name of the memory range
   *
   * @throws Memory_range_exception
   *   IIf (range.addr_start() > range.addr_end()) or
   *       ((range.addr_end() - range.addr_start() + 1) > max_size())
   */
  Fixed_memory_range(Memory_range&& range, const char* name);

  /**
   * Deleted default constructor
   */
  Fixed_memory_range() = delete;

  /**
   * Default copy constructor
   */
  Fixed_memory_range(const Fixed_memory_range&) = default;

  /**
   * Default move constructor
   */
  Fixed_memory_range(Fixed_memory_range&&) = default;

  /**
   * Default copy assignment operator
   */
  Fixed_memory_range& operator=(const Fixed_memory_range&) = default;

  /**
   * Default move assignment operator
   */
  Fixed_memory_range& operator=(Fixed_memory_range&&) = default;

  /**
   * Default destructor
   */
  ~Fixed_memory_range() = default;

  /**
   * Check if the specified memory range is valid
   *
   * @param range
   *   The memory range to check for validity
   *
   * @return true if the specified memory range is valid,
   * false otherwise
   */
  static bool is_valid_range(const Memory_range& range) noexcept;

  /**
   * Check if the specified bounds are valid for describing a
   * memory range
   *
   * @param begin
   *   The beginning address of the memory range
   *
   * @param end
   *   The end address of the memory range
   *
   * @return true if the specified bounds are valid for describing a
   * memory range, false otherwise
   */
  static constexpr bool is_valid_range(const uintptr_t begin, const uintptr_t end) noexcept
  { return (begin < end) or ((end - begin + 1) <= static_cast<uintptr_t>(max_size())); }

  /**
   * Get the maximum number of bytes a memory range is able to hold
   *
   * @return The maximum number of bytes a memory range is able to hold
   */
  static constexpr size_type max_size() noexcept
  { return std::numeric_limits<size_type>::max(); }

  /**
   * Set an operation to perform when bytes_in_use is called
   *
   * @param in_use_operation
   *   An operation to perform when bytes_in_use is called
   */
  void set_in_use_delg(In_use_delg in_use_operation)
  { in_use_op_ = in_use_operation; }

  /**
   * Get the number of bytes in the memory range
   *
   * @return The number of bytes in the memory range
   */
  size_type size() const noexcept
  { return range_.size(); }

  /**
   * Get the name of the memory range
   *
   * @return The name of the memory range
   */
  const char* name() const noexcept
  { return name_; }

  /**
   * The start address of the memory range
   *
   * @return The start address of the memory range
   */
  uintptr_t addr_start() const noexcept {
    return reinterpret_cast<uintptr_t>(range_.data());
  }

  /**
   * The end address of the memory range
   *
   * @return The end address of the memory range
   */
  uintptr_t addr_end() const noexcept
  { return reinterpret_cast<uintptr_t>(range_.data() + range_.size() - 1); }

  /**
   * Check if the specified address is within the memory range
   *
   * @param addr
   *   The specified address to check if it's within the memory range
   *
   * @return true if the specified address is within the memory range,
   * false otherwise
   */
  bool in_range(const uintptr_t addr) const noexcept
  { return (addr >= addr_start()) and (addr <= addr_end()); }

  /**
   * Check if the specified memory range overlaps
   *
   * @param other
   *   The specified memory range to check if it overlaps
   *
   * @return true if the specified memory range overlaps, false otherwise
   */
  bool overlaps(const Fixed_memory_range& other) const noexcept;

  /**
   * Calls the (possibly) user provided delegate showing the number of bytes
   * actually in use by this range
   *
   * E.g. heap has a fixed range, but dynamic use
   *
   * @return The number of bytes actually in use by this range
   */
  size_type bytes_in_use() const
  { return in_use_op_ ? in_use_op_() : size(); }

  /**
   * Get a string representation of this class
   *
   * @return A string representation of this class
   */
  std::string to_string() const;

  /**
   * Operator to transform this class into string form
   */
  operator std::string () const;

  /**
   * Expand or shrink the memory range by the specified size
   * in bytes
   *
   * @expects size > 0
   *
   * @param size
   *   The specified size in bytes to expand or shrink the memory
   *   range by
   *
   * @return The new size of the memory range in bytes
   */
  ptrdiff_t resize(const ptrdiff_t size);

  /**
   * Get the raw representation of the memory range
   *
   * @return The raw representation of the memory range
   */
  const Memory_range& range() const noexcept
  { return range_; }

  /**
   * Operator to transform this class into its raw representation
   */
  operator Memory_range() const
  { return range_; }

  /**
   * Get an iterator to the beginning of the memory range
   *
   * @return An iterator to the beginning of the memory range
   */
  auto begin() noexcept
  { return range_.begin(); }

  /**
   * Get an iterator to the end of the memory range
   *
   * @return An iterator to the end of the memory range
   */
  auto end() noexcept
  { return range_.end(); }

  /**
   * Get a const iterator to the beginning of the memory range
   *
   * @return A const iterator to the beginning of the memory range
   */
  auto cbegin() const noexcept
  { return range_.cbegin(); }

  /**
   * Get a const iterator to the end of the memory range
   *
   * @return A const iterator to the end of the memory range
   */
  auto cend() const noexcept
  { return range_.cend(); }
private:
  Memory_range range_;
  const char*  name_;
  In_use_delg  in_use_op_;
}; //< class Fixed_memory_range

/**
 * This class is a representation of the operating system's
 * memory map
 */
class Memory_map {
public:
  using Key = uintptr_t;
  using Map = std::map<Key, Fixed_memory_range>;

  /**
   * Assign a fixed range of memory to a named purpose
   *
   * @note The requested range cannot overlap with an existing range
   * @note The return value is the only way to access a mutable version of this range
   */
  Fixed_memory_range& assign_range(Fixed_memory_range&& rng);

  /**
   * Assign a memory range of a certain size to a named purpose
   */
  Fixed_memory_range& assign_range(const Fixed_memory_range::size_type size);

  /**
   * Check if an address is within a range in the map
   *
   * @param addr
   *   The address to check
   *
   * @return The key (e.g start address) of the range or 0 if no match
   */
  Key in_range(const Key addr);

  /**
   * Get a reference to the memory range associated with the specified
   * key
   *
   * @param key
   *   The key associated with the memory range
   *
   * @return A reference to the memory range associated with the
   * specified key
   *
   * @throws Memory_range_exception IIf (key == 0)
   * @throws std::out_of_range IIf (key < 0) or (key >= size())
   */
  Fixed_memory_range& at(const Key key);

  /**
   * Get a const reference to the memory range associated with the
   * specified key
   *
   * @param key
   *   The key associated with the memory range
   *
   * @return A const reference to the memory range associated with the
   * specified key
   *
   * @throws Memory_range_exception IIf (key == 0)
   * @throws std::out_of_range IIf (key < 0) or (key >= size())
   */
  const Fixed_memory_range& at(const Key key) const;

  /**
   * Resize a range if possible (e.g. if the range.bytes_in_use() was less than its size)
   *
   * @param size
   *   The new size for the specified memory range
   *
   * @return The new size for the specified memory range
   *
   * @throws std::out_of_range IIf (key <= 0) or (key >= size())
   * @throws Memory_range_exception IIf a resize wasn't possible
   */
  ptrdiff_t resize(const Key key, const ptrdiff_t size);

  /**
   * Get the number of memory ranges in the memory map
   *
   * @return The number of memory ranges in the memory map
   */
  auto size() const noexcept
  { return map_.size(); }

  /**
   * Check if the memory map is empty
   *
   * @return true if the memory map is empty, false otherwise
   */
  auto empty() const noexcept
  { return map_.empty(); }

  /**
   * Get the raw representation of the memory map
   *
   * @return The raw representation of the memory map
   */
  const Map& map() const noexcept
  { return map_; }

  void erase(Key k) {
    map_.erase(k);
  }

  void clear() {
    map_.clear();
  }

  /**
   * Operator to transform this class into its raw representation
   */
  operator const Map&() const noexcept
  { return map_; }

  /**
   * Get an iterator to the beginning of the memory map
   *
   * @return An iterator to the beginning of the memory map
   */
  auto begin() noexcept
  { return map_.begin(); }

  /**
   * Get an iterator to the end of the memory map
   *
   * @return An iterator to the end of the memory map
   */
  auto end() noexcept
  { return map_.end(); }

  /**
   * Get a const iterator to the beginning of the memory map
   *
   * @return A const iterator to the beginning of the memory map
   */
  auto cbegin() const noexcept
  { return map_.cbegin(); }

  /**
   * Get a const iterator to the end of the memory map
   *
   * @return A const iterator to the end of the memory map
   */
  auto cend() const noexcept
  { return map_.cend(); }
private:
  Map map_;
}; //< class Memory_map

} // ns os::mem
#endif //< KERNEL_MEMMAP_HPP
