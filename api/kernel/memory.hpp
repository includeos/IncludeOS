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

#include <common>
#include <util/bitops.hpp>
#include <util/units.hpp>
#include <cstdlib>
#include <sstream>

namespace os {
namespace mem {
  using namespace util::literals;

  /** POSIX mprotect compliant access bits **/
  enum class Access : uint8_t {
    none = 0,
    read = 1,
    write = 2,
    execute = 4
  };

  /** Get bitfield with bit set for each supported page size */
  uintptr_t supported_page_sizes();

  /** Get the smallest supported page size */
  uintptr_t min_psize();

  /** Get the largest supported page size */
  uintptr_t max_psize();

  /** Determine if size is a supported page size */
  bool supported_page_size(uintptr_t size);

  /** A-aligned allocation of T **/
  template <typename T, int A, typename... Args>
  T* aligned_alloc(Args... a)
  {
    void* ptr;
    if (posix_memalign(&ptr, A, sizeof(T)) > 0)
      return nullptr;
    return new (ptr) T(a...);
  }


  inline std::string page_sizes_str(size_t bits){

    if (bits == 0) return "None";

    std::stringstream out;
    while (bits){
      auto ps = 1 << (__builtin_ffsl(bits) - 1);
      bits &= ~ps;
      out << util::Byte_r(ps);
      if (bits)
        out << ", ";
    }

    return out.str();
  }

  template <typename Fl = Access>
  struct Mapping {

    static const size_t any_size;

    uintptr_t lin {};
    uintptr_t phys {};
    Fl flags {};
    size_t size = 0;
    size_t page_sizes = 0;

    Mapping() = default;
    Mapping(uintptr_t linear, uintptr_t physical, Fl fl, size_t sz)
      : lin{linear}, phys{physical}, flags{fl}, size{sz},
        page_sizes{any_size} {}

    Mapping(uintptr_t linear, uintptr_t physical, Fl fl, size_t sz, size_t psz)
      : lin{linear}, phys{physical}, flags{fl}, size{sz}, page_sizes{psz}
    {}

    operator bool() const noexcept
    { return size != 0 && page_sizes !=0; }


    bool operator==(const Mapping& rhs) const noexcept
    { return lin == rhs.lin
        && phys == rhs.phys
        && flags == rhs.flags
        && size == rhs.size
        && page_sizes == rhs.page_sizes; }

    bool operator!=(const Mapping& rhs) const noexcept
    { return ! (*this == rhs); }

    inline Mapping operator+(const Mapping& rhs) noexcept;

    Mapping operator+=(const Mapping& rhs) noexcept {
      *this = *this + rhs;
      return *this;
    }

    size_t page_count() const noexcept
    { return page_sizes ? (size + page_sizes - 1) / page_sizes : 0; }

    // Smallest page size in map
    size_t min_psize() const noexcept
    { return util::bits::keepfirst(page_sizes); }

    // Largest page size in map
    size_t lmax_psize() const noexcept
    { return util::bits::keeplast(page_sizes); }

    std::string to_string() const
    {
      std::stringstream out;
      out << "0x" << std::hex << lin << "->" << phys
          << ", size: "<< util::Byte_r(size)
          << " flags: 0x" << std::hex << (int)flags << std::dec;

      if (util::bits::is_pow2(page_sizes)) {
        out << std::dec << " ( " << page_count()
            << " x " << util::Byte_r(page_sizes)  << " pages )";
      } else {
        out << " page sizes: " << page_sizes_str(page_sizes);
      }



      return out.str();
    }
  };

  template <typename Fl>
  inline std::ostream& operator<<(std::ostream& out, const Mapping<Fl>& m)
  {
    return out << m.to_string();
  }

  using Map = Mapping<>;

  class Memory_exception : public std::runtime_error
  { using runtime_error::runtime_error; };


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
    Mapping res;

    if (! rhs) {
      return *this;
    }

    if (! *this)
      return rhs;

    if (res == rhs)
      return res;

    res.lin  = std::min(lin, rhs.lin);
    res.phys = std::min(phys, rhs.phys);

    // The mappings must have connecting ranges
    if ((rhs and rhs.lin + rhs.size != lin)
        and (*this and lin + size != rhs.lin))
    {
      Ensures(!res);
      return res;
    }

    res.page_sizes |= rhs.page_sizes;

    // The mappings can span several page sizes
    if (page_sizes && page_sizes != rhs.page_sizes)
    {
      res.page_sizes |= page_sizes;
    }

    res.size = size + rhs.size;
    res.flags = flags & rhs.flags;

    if (rhs)
      Ensures(res);

    return res;
  }
}}

#endif
