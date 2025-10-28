#ifndef	MEM_FLAGS_HPP
#define	MEM_FLAGS_HPP

#include <cstdint>
#include <util/bitops.hpp>

namespace os::mem {
  enum class Access : uint8_t {
      none = 0,
      read = 1,
      write = 2,
      execute = 4
    };
} // os::mem

namespace util {
  inline namespace bitops {
    template<>
    struct enable_bitmask_ops<os::mem::Access> {
      using type = typename std::underlying_type<os::mem::Access>::type;
      static constexpr bool enable = true;
    };
  }
}

#endif // MEM_FLAGS_HPP
