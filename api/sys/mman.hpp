/*
 * provides namespaced types for `sys_mman(0p)` values
 */
#ifndef	_SYS_MMAN_HPP
#define	_SYS_MMAN_HPP

#include <cstdint>
#include <sys/mman.h>
#include <util/bitops.hpp>
#include <type_traits>

namespace os::mem {
  enum class Flags : uint8_t {
    None      = 0,
    Shared    = MAP_SHARED,
    Private   = MAP_PRIVATE,
    Fixed     = MAP_FIXED,
    Anonymous = MAP_ANONYMOUS,
  };
} // os::mmap


namespace util {
  inline namespace bitops {
    template<> struct enable_bitmask_ops<os::mem::Flags> {
      using type = std::underlying_type<os::mem::Flags>::type;
      static constexpr bool enable = true;
    };
  }

