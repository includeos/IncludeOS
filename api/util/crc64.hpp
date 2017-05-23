// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 the "License";
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
#ifndef UTIL_CRC64_HPP
#define UTIL_CRC64_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace util {

template<uint64_t POLY>
struct crc64 {
  explicit crc64(const std::string& data) noexcept
    : crc64(data.data(), data.size())
  {}

  explicit crc64(const std::vector<char>& data) noexcept
    : crc64(data.data(), data.size())
  {}

  template<size_t N>
  explicit crc64(const std::array<char, N>& data) noexcept
    : crc64(data.data(), data.size())
  {}

  constexpr explicit crc64(const char* data, size_t data_len) noexcept {
    crc_register_ = checksum(0, data, data_len);
  }


  static constexpr uint64_t checksum(uint64_t crc_accumulator, const char* data, size_t data_len) noexcept {
    auto crc64_table = get_table();

    crc_accumulator = ~crc_accumulator;

    while (data_len > 8) {
      crc_accumulator ^= static_cast<uint64_t>(data[0])       | static_cast<uint64_t>(data[1]) << 8  |
                         static_cast<uint64_t>(data[2]) << 16 | static_cast<uint64_t>(data[3]) << 24 |
                         static_cast<uint64_t>(data[4]) << 32 | static_cast<uint64_t>(data[5]) << 40 |
                         static_cast<uint64_t>(data[6]) << 48 | static_cast<uint64_t>(data[7]) << 56;

      crc_accumulator = crc64_table[7][crc_accumulator & 0xff]         ^
                        crc64_table[6][(crc_accumulator >> 8)  & 0xff] ^
                        crc64_table[5][(crc_accumulator >> 16) & 0xff] ^
                        crc64_table[4][(crc_accumulator >> 24) & 0xff] ^
                        crc64_table[3][(crc_accumulator >> 32) & 0xff] ^
                        crc64_table[2][(crc_accumulator >> 40) & 0xff] ^
                        crc64_table[1][(crc_accumulator >> 48) & 0xff] ^
                        crc64_table[0][crc_accumulator  >> 56];

      data_len -= 8;
    }

    for (size_t i = 0; i < data_len; ++i) {
      crc_accumulator = crc64_table[0][static_cast<uint8_t>(crc_accumulator) ^ data[i]] ^ (crc_accumulator >> 8);
    }

    return ~crc_accumulator;
  }

  constexpr operator uint64_t() const noexcept
  { return crc_register_; }

private:
  uint64_t crc_register_ {0};

  using CRC64_table_t = std::array<std::array<uint64_t, 256>, 8>;

  static constexpr const CRC64_table_t get_table() noexcept {
    auto crc64_itable = get_init_table();

    CRC64_table_t crc64_table {{}};
    crc64_table[0] = crc64_itable;

    for (uint64_t i = 0; i < 256; ++i) {
      auto crc_accumulator = crc64_itable[i];

      for (uint64_t j = 1; j < 8; ++j) {
        crc_accumulator = crc64_itable[crc_accumulator & 0xff] ^ (crc_accumulator >> 8);
        crc64_table[j][i] = crc_accumulator;
      }
    }

    return crc64_table;
  }

  static constexpr const std::array<uint64_t, 256> get_init_table() noexcept {
    std::array<uint64_t, 256> data {{}};

    for (uint64_t i {0}; i < 256; ++i) {
      uint64_t crc {i};

      for (uint64_t j {0}; j < 8; ++j) {
        crc = (crc & 1) ? ((crc >> 1) ^ POLY) : (crc >> 1);
      }

      data[i] = crc;
    }

    return data;
  }
}; //< struct crc64

using crc64_iso_checksum  = crc64<0xD800000000000000>; //< Specified in ISO 3309
using crc64_ecma_checksum = crc64<0xC96C5795D7870F42>; //< Specified in ECMA 182

} //< namespace util

#endif //< UTIL_CRC64_HPP
