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

#include <sstream>
#include <iomanip>
#include <kernel/memmap.hpp>
#include <util/units.hpp>

using namespace os::mem;

///////////////////////////////////////////////////////////////////////////////
Fixed_memory_range::Fixed_memory_range(const uintptr_t begin, const uintptr_t end, const char* name,
                                       In_use_delg in_use_operation)
  : name_{name}
  , in_use_op_{in_use_operation}
{
  if (begin > end) {
    throw Memory_range_exception{"Start is larger than end: " + std::to_string(begin) + " > " + std::to_string(end)};
  }

  if ((end - begin + 1) > static_cast<uintptr_t>(max_size())) {
    throw Memory_range_exception{"Maximum range size is " + std::to_string(max_size())};
  }

  range_ = Memory_range(reinterpret_cast<uint8_t*>(begin), (end - begin + 1));
}

///////////////////////////////////////////////////////////////////////////////
Fixed_memory_range::Fixed_memory_range(Memory_range&& range, const char* name)
  : range_{std::move(range)}
  , name_{name}
{
  if (not is_valid_range(range_)) {
    throw Memory_range_exception{"The specified range is invalid"};
  }
}


///////////////////////////////////////////////////////////////////////////////
bool Fixed_memory_range::is_valid_range(const Memory_range& range) noexcept {
  const auto start = reinterpret_cast<uintptr_t>(range.data());
  const auto end   = reinterpret_cast<uintptr_t>(range.data() + range.size() - 1);
  return (start < end) and ((end - start + 1) <= static_cast<uintptr_t>(max_size()));
}

///////////////////////////////////////////////////////////////////////////////
ptrdiff_t Fixed_memory_range::resize(const ptrdiff_t size) {
  Expects(size > 0);
  range_ = Memory_range{range_.data(), size};
  return size;
}

///////////////////////////////////////////////////////////////////////////////
bool Fixed_memory_range::overlaps(const Fixed_memory_range& other) const noexcept {
  return (in_range(other.addr_start()) or in_range(other.addr_end()))   //< Other range overlaps with my range
      or (other.in_range(addr_start()) and other.in_range(addr_end())); //< My range is inside other range
}

///////////////////////////////////////////////////////////////////////////////
std::string Fixed_memory_range::to_string() const {
  std::stringstream out;
  auto pct_used = (double(bytes_in_use()) / size()) * 100;

  out << std::hex  << std::setfill('0')
      << "0x"      << std::setw(10)     << addr_start()  << " - "
      << "0x"      << std::setw(10)     << addr_end()    << ", "
      << std::dec  << std::setfill(' ') << std::setw(11) << util::Byte_r(size())
      << " (";

  if (pct_used == 100 or pct_used < 0.001)
    out << std::setprecision(1);
  else
    out << std::setprecision(3);

  out << std::fixed << std::setw(6) << std::left
      << pct_used << "%) "
      <<  std::setw(25) << name_;

  return out.str();
}


///////////////////////////////////////////////////////////////////////////////
Fixed_memory_range::operator std::string () const {
  return to_string();
}

///////////////////////////////////////////////////////////////////////////////
Memory_map::Key Memory_map::in_range(const Key addr) {
  if (UNLIKELY(map_.empty())) {
    return 0;
  }

  auto closest_above = map_.lower_bound(addr);

  // This might be an exact match to the beginning of the range
  if (UNLIKELY(closest_above->first == addr)) {
    return closest_above->first;
  }

  if ((closest_above == map_.end()) or (closest_above not_eq map_.begin())) {
    closest_above--;
  }

  if (closest_above->second.in_range(addr)) {
    return closest_above->first;
  }

  // NOTE:  0 is never a valid range since a span can't start at address 0
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
Fixed_memory_range& Memory_map::assign_range(Fixed_memory_range&& rng) {
  debug("* Assgning range %s", rng.to_string().c_str());

  // Keys are address representations, not pointers
  const auto key = reinterpret_cast<uintptr_t>(rng.range().data());

  if (UNLIKELY(map_.empty())) {
    auto new_entry  = map_.emplace(key, std::move(rng));
    debug("* Range inserted (success: 0x%x, %i)", new_entry.first->first, new_entry.second);
    return new_entry.first->second;
  }

  // Make sure the range does not overlap with any existing ranges
  auto closest_match = map_.lower_bound(key);

  // If the new range starts above all other ranges, we still need to make sure
  // the last element doesn't overlap
  if (closest_match == map_.end()) {
    closest_match--;
  }

  if (UNLIKELY(rng.overlaps(closest_match->second))) {
    throw Memory_range_exception{"Range '"+ std::string(rng.name())
                                 + "' overlaps with the range above requested addr:  "
                                 + closest_match->second.to_string()};
  }

  // We also need to check the preceeding entry, if any, or if the closest match was above us
  if (UNLIKELY((closest_match not_eq map_.begin()) and (closest_match->second.addr_start() > key))) {
    closest_match--;

    if (UNLIKELY(rng.overlaps(closest_match->second))) {
      throw Memory_range_exception("Range '"+ std::string(rng.name())
                                   + "' overlaps with the range below requested addr: "
                                   + closest_match->second.to_string());
    }
  }

  auto new_entry = map_.emplace(key, std::move(rng));

  debug("* Range inserted (success: %i)", new_entry.second);

  return new_entry.first->second;
}

///////////////////////////////////////////////////////////////////////////////
Fixed_memory_range& Memory_map::at(const Key key) {
  return const_cast<Fixed_memory_range&>(static_cast<const Memory_map*>(this)->at(key));
}

///////////////////////////////////////////////////////////////////////////////
const Fixed_memory_range& Memory_map::at(const Key key) const {
  if (UNLIKELY(key == 0)) {
    throw Memory_range_exception{"No range starts at address 0"};
  }

  return map_.at(key);
}

///////////////////////////////////////////////////////////////////////////////
ptrdiff_t Memory_map::resize(const Key key, const ptrdiff_t size) {
  auto& range = map_.at(key);

  debug("Resize range 0x%x using %lib to use %lib", key, range.bytes_in_use(), size);

  if (range.bytes_in_use() >= size) {
    throw Memory_range_exception{"Can't resize. Range " + std::string(range.name())
                                 + " uses " + std::to_string(range.bytes_in_use())
                                 + "b, more than the requested " + std::to_string(size) + "b"};
  }

  auto collision = in_range(range.addr_start() + size);
  if (collision and (collision not_eq range.addr_start())) {
    throw Memory_range_exception{"Can't resize. Range would collide with "
                                 + at(collision).to_string()};
  }

  return range.resize(size);
}
