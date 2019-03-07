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

#ifndef UTIL_BITOPS_HPP
#define UTIL_BITOPS_HPP

#include <type_traits>
#include <cstdint>

namespace util {
inline namespace bitops {

//
// Enabling bitmask ops for enum class etc.
//

template<typename E>
struct enable_bitmask_ops{
  using type = typename std::underlying_type<E>::type;
  static constexpr bool enable=false;
};

template<typename E, typename F>
constexpr typename std::enable_if<enable_bitmask_ops<E>::enable,E>::type
operator|(E lhs,F rhs){
  using type = typename enable_bitmask_ops<E>::type;
  return static_cast<E>(static_cast<type>(lhs) | static_cast<type>(rhs));
}

template<typename E, typename F>
typename std::enable_if<enable_bitmask_ops<E>::enable,E>::type
constexpr operator&(E lhs,F rhs){
  using type = typename enable_bitmask_ops<E>::type;
  return static_cast<E>(static_cast<type>(lhs) & static_cast<type>(rhs));
}

template<typename E, typename F>
typename std::enable_if<enable_bitmask_ops<E>::enable,E>::type
constexpr operator^(E lhs,F rhs){
  using type = typename enable_bitmask_ops<F>::type;
  return static_cast<E>(static_cast<type>(lhs) ^ static_cast<type>(rhs));
}

template<typename E, typename F>
constexpr typename std::enable_if<enable_bitmask_ops<E>::enable,E>::type
operator|=(E& lhs,F rhs){
  using type = typename enable_bitmask_ops<F>::type;
  lhs = static_cast<E>(static_cast<type>(lhs) | static_cast<type>(rhs));
  return lhs;
}

template<typename E, typename F>
constexpr typename std::enable_if<enable_bitmask_ops<E>::enable,E>::type&
operator&=(E& lhs,F rhs){
  using type = typename enable_bitmask_ops<F>::type;
  lhs = static_cast<E>(static_cast<type>(lhs) & static_cast<type>(rhs));
  return lhs;
}

template<typename E, typename F>
constexpr typename std::enable_if<enable_bitmask_ops<E>::enable,E>::type&
operator^=(E& lhs,F rhs){
  using type = typename enable_bitmask_ops<E>::type;
  lhs = static_cast<E>(static_cast<type>(lhs) ^ static_cast<type>(rhs));
  return lhs;
}

template<typename E>
typename std::enable_if<enable_bitmask_ops<E>::enable, E>::type
constexpr operator~(E flag){
  using base_type = typename enable_bitmask_ops<E>::type;
  return static_cast<E>(~static_cast<base_type>(flag));
}


template<typename E>
constexpr typename std::enable_if<enable_bitmask_ops<E>::enable, bool>::type
has_flag(E flag){
  using base_type = typename std::underlying_type<E>::type;
  return static_cast<base_type>(flag);
}

template<typename E>
constexpr typename std::enable_if<enable_bitmask_ops<E>::enable, bool>::type
has_flag(E field, E flags){
  return (field & flags) == flags ;
}

template<>
struct enable_bitmask_ops<uintptr_t> {
  using type = uintptr_t;
  static constexpr bool enable = true;
};
}

namespace bits {

//
// Various bit operations
//
// Number of bits per word
constexpr int bitcnt()
{ return sizeof(uintptr_t) * 8; }

// Count leading zeroes
inline uintptr_t clz(uintptr_t n){
  return __builtin_clzl(n);
}

// Count trailing zeroes
inline uintptr_t ctz(uintptr_t n){
  return __builtin_ctzl(n);
}

// Find first bit set.
inline uintptr_t ffs(uintptr_t n){
  return __builtin_ffsl(n);
}

// Find last bit set. The integral part of log2(n).
inline uintptr_t fls(uintptr_t n){
  return (uintptr_t) bitcnt() - clz(n);
}

// Keep last set bit, clear the rest.
inline uintptr_t keeplast(uintptr_t n)
{ return 1LU << (fls(n) - 1); }

// Keep first set bit, clear the rest.
inline uintptr_t keepfirst(uintptr_t n)
{ return 1LU << (ffs(n) - 1); }

// Number of bits set in n
inline uintptr_t popcount(uintptr_t n)
{ return __builtin_popcountl(n); }

// Check for power of two
inline constexpr bool is_pow2(uintptr_t i)
{ return i && !(i & (i - 1)); }

inline uintptr_t next_pow2(uintptr_t i)
{
  auto lastbit = keeplast(i);
  return i == lastbit ? i : lastbit * 2;
}

// Multiples of M required to cover x
template <uintptr_t M>
inline uintptr_t multip(uintptr_t x)
{ return (M - 1 + x) / M; }

inline constexpr uintptr_t multip(uintptr_t M, uintptr_t x)
{return (M - 1 + x) / M; }

// Round up to nearest multiple of M
template <uintptr_t M>
inline uintptr_t roundto(uintptr_t x)
{ return multip<M>(x) * M; }

inline constexpr uintptr_t roundto(uintptr_t M, uintptr_t x)
{ return multip(M,x) * M; }

inline constexpr uintptr_t align(uintptr_t M, uintptr_t x)
{ return roundto(M, x); }

// Determine if ptr is A-aligned
template <uintptr_t A>
bool is_aligned(uintptr_t ptr) noexcept
{
  return (ptr & (A - 1)) == 0;
}

template <uintptr_t A>
bool is_aligned(void* ptr) noexcept
{
  return is_aligned<A>(reinterpret_cast<uintptr_t>(ptr));
}

inline bool is_aligned(uintptr_t A, uintptr_t ptr) noexcept
{
  return (ptr & (A - 1)) == 0;
}

inline size_t upercent(size_t a, size_t b) noexcept
{
  return (100 * a + b / 2) / b;
}

} // ns bitops
} // ns util

#endif
