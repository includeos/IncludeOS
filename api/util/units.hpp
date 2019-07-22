// -*-C++-*-

#pragma once
#ifndef UTIL_UNITS_HPP
#define UTIL_UNITS_HPP

#include <chrono>
#include <iostream>

namespace util {
inline namespace literals
{

  /**
   * Memory size literals
   */
  using int_t = unsigned long long;

  constexpr auto operator"" _b ( int_t x)
  { return 1ULL * x; }

  constexpr auto operator"" _KiB ( int_t x)
  { return 1024_b * x; }

  constexpr auto operator"" _MiB ( int_t x )
  { return 1024_KiB * x; }

  constexpr auto operator"" _GiB ( int_t x )
  { return 1024_MiB *  x; }

  constexpr auto operator"" _TiB ( int_t x )
  { return 1024_GiB *  x; }



  /**
   * Frequency literals
   */
  using Hz  = std::chrono::duration<double>;
  using KHz = std::chrono::duration<double, std::kilo>;
  using MHz = std::chrono::duration<double, std::mega>;
  using GHz = std::chrono::duration<double, std::giga>;

  constexpr Hz operator"" _hz(long double d) {
    return Hz(d);
  }

  constexpr KHz operator"" _khz(long double d) {
    return KHz(d);
  }

  constexpr MHz operator"" _mhz(long double d) {
    return MHz(d);
  }

  constexpr GHz operator"" _ghz(long double d) {
    return GHz(d);
  }

} // ns literals

/** String representation of bytes as KiB, MiB, GiB **/
struct Byte_r {
  Byte_r(uint64_t b) : b_{b}{}

  std::string to_string() const {
    char frep[20];
    double dbl = static_cast<double>(b_);
    if (b_ >= 1_TiB) {
      sprintf(frep, "%0.3f", dbl / 1_TiB);
      return std::string(frep) + "_TiB";
    }
    if (b_ >= 1_GiB) {
      sprintf(frep, "%0.3f", dbl / 1_GiB);
      return std::string(frep) + "_GiB";
    }
    if (b_ >= 1_MiB) {
      sprintf(frep, "%0.3f", dbl / 1_MiB);
      return std::string(frep) + "_MiB";
    }
    if (b_ >= 1_KiB) {
      sprintf(frep, "%0.3f", dbl / 1_KiB);
      return std::string(frep) + "_KiB";
    }

    return std::to_string(b_) + "_b";
  }

  uint64_t b_{};
};

inline std::ostream& operator <<(std::ostream& out, const Byte_r& b){
  return out << b.to_string();
}


} // ns util

#endif
