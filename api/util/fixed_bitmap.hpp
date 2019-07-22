
#pragma once
#ifndef UTIL_FIXED_BITMAP_HPP
#define UTIL_FIXED_BITMAP_HPP

#include "membitmap.hpp"
#include <array>

/**
 * @brief      A membitmap with a fixed amount of bits and storage.
 *
 * @tparam     N     Number of bits. Needs to be divisable by sizeof(Storage)
 */
template <size_t N>
class Fixed_bitmap : public MemBitmap {
public:
  using Storage = MemBitmap::word;
  static_assert(N >= sizeof(Storage), "Number of bits need to be atleast sizeof(Storage)");
  static_assert(N % sizeof(Storage) == 0, "Number of bits need to be divisable by sizeof(Storage)");

public:
  Fixed_bitmap() :
    MemBitmap{},
    storage{}
  {
    set_location(storage.data(), N / sizeof(Storage));
  }

private:
  std::array<Storage, N / sizeof(Storage)> storage;

}; // < class Fixed_bitmap

#endif
