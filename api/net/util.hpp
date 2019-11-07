
#ifndef NET_UTIL_HPP
#define NET_UTIL_HPP

#include <cstdint>
#include <type_traits>

namespace net {

  /*
   * See P.49 of C programming
   * Get "n" bits from integer "x", starting from position "p"
   * e.g., getbits(x, 31, 8) -- highest byte
   *       getbits(x,  7, 8) -- lowest  byte
   */
  template
  <
    typename T,
    typename = std::enable_if_t<std::is_integral<T>::value>
  >
  constexpr inline auto getbits(const T x, const T p, const T n) noexcept {
    return (x >> ((p + 1) - n)) & ~(~0 << n);
  }

  #ifndef ntohs
  constexpr inline uint16_t ntohs(const uint16_t n) noexcept {
    return __builtin_bswap16(n);
  }
  #endif

  #ifndef htons
  constexpr inline uint16_t htons(const uint16_t n) noexcept {
    return __builtin_bswap16(n);
  }
  #endif

  #ifndef ntohl
  constexpr inline uint32_t ntohl(const uint32_t n) noexcept {
    return __builtin_bswap32(n);
  }
  #endif

  #ifndef htonl
  constexpr inline uint32_t htonl(const uint32_t n) noexcept {
    return __builtin_bswap32(n);
  }
  #endif

  #ifndef ntohll
  constexpr inline uint64_t ntohll(const uint64_t n) noexcept {
    return __builtin_bswap64(n);
  }
  #endif

  #ifndef htonll
  constexpr inline uint64_t htonll(const uint64_t n) noexcept {
    return __builtin_bswap64(n);
  }
  #endif

} //< namespace net

#endif //< NET_UTIL_HPP
