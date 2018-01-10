// -*- C++ -*-
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

#ifndef KERNEL_MEMORY_HPP
#define KERNEL_MEMORY_HPP

#define DEBUG 1

#include <common>
#include <util/bitops.hpp>
#include <util/units.hpp>
#include <cstdlib>
#include <sstream>

namespace os {
namespace mem {

  /** POSIX mprotect compliant access bits **/
  enum class Access : uint8_t {
    none = 0,
    read = 1,
    write = 2,
    execute = 4
  };

  /** A-aligned allocation of T **/
  template <typename T, int A, typename... Args>
  T* aligned_alloc(Args... a)
  {
    void* ptr;
    if (posix_memalign(&ptr, A, sizeof(T)) > 0)
      return nullptr;
    return new (ptr) T(a...);
  }

  /** Determine if ptr is A-aligned **/
  template <uintptr_t A>
  bool is_aligned(uintptr_t ptr)
  {
    return (ptr & (A - 1)) == 0;
  }

  template <uintptr_t A>
  bool is_aligned(void* ptr)
  {
    return is_aligned<A>(reinterpret_cast<uintptr_t>(ptr));
  }

  template <typename Fl = Access>
  struct Mapping {
    uintptr_t lin {};
    uintptr_t phys {};
    Fl flags {};
    size_t size = 0;
    size_t page_size = 4096;

    Mapping() = default;

    operator bool() const noexcept
    { return size != 0 && page_size !=0; }


    bool operator==(const Mapping& rhs) const noexcept
    { return lin == rhs.lin
        && phys == rhs.phys
        && flags == rhs.flags
        && size == rhs.size
        && page_size == rhs.page_size; }

    bool operator!=(const Mapping& rhs) const noexcept
    { return ! *this == rhs; }

    inline Mapping operator+(const Mapping& rhs) noexcept;

    Mapping operator+=(const Mapping& rhs) noexcept {
      *this = *this + rhs;
      return *this;
    }

    size_t page_count() const noexcept
    { return page_size ? (size + page_size - 1) / page_size : 0; }

    std::string to_string() const
    {
      std::stringstream out;
      out << "0x" << std::hex << lin << "->" << phys
          << ", size: "<< util::Byte_r(size)
          << std::dec << " ( " << page_count()
          << " x " << util::Byte_r(page_size)  << " pages )"
          << " flags: 0x" << std::hex << (int)flags << std::dec;

      return out.str();
    }
  };

  template <typename Fl>
  inline std::ostream& operator<<(std::ostream& out, const Mapping<Fl>& m)
  {
    return out << m.to_string();
  }

  using Map = Mapping<>;

  /**
   * Map linear/virtual address to free physical memory, with flags access.
   * If the address is already mapped the old mapping is overwritten
   * but page directories preserved
   */
  Map map(uintptr_t linear, size_t len, Access flags);

  /**
   * Map linear address to physical memory, according to provided Mapping.
   * Provided map.page_size will be ignored, but the returned map.page_size
   * will have one bit set for each page size used
   */
  Map map(Map, const char* name = "mem::map");

  /**
   * Unmap the memory mapped by linear address,
   * effectively freeing the underlying physical memory.
   * The behavior is undefined if addr was not mapped with a call to map
   **/
  Map unmap(uintptr_t addr);

  /** Get protection flags for page containing a given address */
  Access flags(uintptr_t addr);

  /** Determine active page size of a given linear address **/
  uintptr_t active_page_size(uintptr_t addr);
  uintptr_t active_page_size(void* addr);

  /**
   * Set and return access flags for a given linear address range
   * The range is expected to be mapped by a previous call to map.
   **/
  Access protect(uintptr_t linear, Access flags = Access::read);

  /** Set and return access flags for a page starting at linear **/
  Access protect_page(uintptr_t linear, Access flags = Access::read);


}
}

namespace util {
inline namespace bitops {
  template<>
  struct enable_bitmask_ops<os::mem::Access> {
    using type = typename std::underlying_type<os::mem::Access>::type;
    static constexpr bool enable = true;
  };
}
}


namespace os {
namespace mem {

  template <typename Fl>
  Mapping<Fl> Mapping<Fl>::operator+(const Mapping& rhs) noexcept
  {
    using namespace util::bitops;
    Mapping m;

    if (! rhs) {
      return *this;
    }

    if (! *this)
      return rhs;

    if (m == rhs)
      return m;

    m.lin  = std::min(lin, rhs.lin);
    m.phys = std::min(phys, rhs.phys);

    // The mappings must have connecting ranges
    if ((rhs and rhs.lin + rhs.size != lin)
        and (*this and lin + size != rhs.lin))
    {
      Ensures(!m);
      return m;
    }

    m.page_size |= rhs.page_size;

    // The mappings can span several page sizes
    if (page_size && page_size != rhs.page_size)
    {
      m.page_size |= page_size;
    }

    // The mappings must have the same flags
    if (flags != Fl::none && flags != rhs.flags)
    {
      Ensures(!m);
      return m;
    }

    m.size = size + rhs.size;
    m.flags = rhs.flags | flags;

    if (rhs)
      Ensures(m);

    return m;
  }
}}

#endif
