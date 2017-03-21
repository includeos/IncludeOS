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

#pragma once
#ifndef UTIL_BASE64_HPP
#define UTIL_BASE64_HPP

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

//
// This module consist of functions for the Base64 encoding scheme
//
// The implementation supports the RFC specified at:
// http://www.ietf.org/rfc/rfc4648.txt
//
// NOTE: Currently the codec don't support MIME's
//
namespace base64 {

/**
 * This type is thrown at the caller of decode
 * upon encountering an error
 */
struct Decode_error : public std::runtime_error {
  using runtime_error::runtime_error;
};

/**
 * This type is used as a switching mechanism to specify
 * which alphabet to use on call
 */
struct url_alphabet final {
  constexpr explicit url_alphabet(const bool choice)
    : use_url_alphabet_{choice}
  {}

  constexpr operator bool() const noexcept
  { return use_url_alphabet_; }

  const bool use_url_alphabet_;
}; //< struct url_alphabet

/**
 * Get the Base64 alphabet used within this module
 *
 * @return The Base64 alphabet used within this module
 */
inline auto base64_alphabet() noexcept {
  return
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "+/";
}

/**
 * Get the Base64URL alphabet used within this module
 *
 * @return The Base64URL alphabet used within this module
 */
inline auto base64url_alphabet() noexcept {
  return
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "-_";
}

/**
 * Encode the specified data into Base64 format
 *
 * @param data
 *   The data to encode into Base64 format
 *
 * @param length
 *   The length of the data as number of char's
 *
 * @param url_alphabet_switch
 *   Whether to use the Base64URL alphabet
 *
 * @return Base64 encoded data
 */
template<typename R = std::string>
R encode(const char* data, const size_t length, const url_alphabet url_alphabet_switch = url_alphabet{false}) {
  if (data == nullptr) return R{};

  const char* const alphabet = url_alphabet_switch ? base64url_alphabet() : base64_alphabet();
  auto encode_unit = [alphabet](const int unit) noexcept -> char {
    return ((unit >= 0) and (unit < 64)) ? alphabet[unit] : unit;
  };

  R buffer;
  buffer.reserve(((length + 2) / 3) * 4);

  for (size_t i = 0; i < length; i += 3) {
    buffer.push_back(encode_unit((data[i] & 0xFC) >> 2));
    int unit = (data[i] & 0x03) << 4;
    if ((i + 1) < length) {
      buffer.push_back(encode_unit(unit | ((data[i + 1] & 0xF0) >> 4)));
      unit = (data[i + 1] & 0x0F) << 2;
      if ((i + 2) < length) {
        buffer.push_back(encode_unit(unit | ((data[i + 2] & 0xC0) >> 6)));
        buffer.push_back(encode_unit(data[i + 2] & 0x3F));
      } else  {
        buffer.push_back(encode_unit(unit));
        buffer.push_back('=');
        break;
      }
    } else {
      buffer.push_back(encode_unit(unit));
      buffer.append("==");
      break;
    }
  }

  return buffer;
}

/**
 * Encode the specified data into Base64 format
 *
 * @param data
 *   The data to encode into Base64 format
 *
 * @param url_alphabet_switch
 *   Whether to use the Base64URL alphabet
 *
 * @return Base64 encoded data
 */
template<typename R = std::string, typename D = std::vector<char>>
inline R encode(const D& data, const url_alphabet url_alphabet_switch = url_alphabet{false}) {
  return encode<R>(data.data(), data.size(), url_alphabet_switch);
}

/**
 * Encode a C-String into Base64 format
 *
 * @param data
 *   The C-String to encode into Base64 format
 *
 * @param url_alphabet_switch
 *   Whether to use the Base64URL alphabet
 *
 * @return Base64 encoded data
 */
template<typename R = std::string>
inline R encode(const char* data, const url_alphabet url_alphabet_switch = url_alphabet{false}) {
  const auto length = std::strlen(data);

  if (length == 0) {
    return R{};
  }

  return encode<R>(data, length, url_alphabet_switch);
}

/**
 * Decode the specified Base64 encoded data
 *
 * @param data
 *  The data in Base64 format to decode
 *
 * @param length
 *   The length of the data as number of char's
 *
 * @param url_alphabet_switch
 *   Whether to use the Base64URL alphabet
 *
 * @return The decoded data
 */
template<typename R = std::vector<char>>
R decode(const char* data, size_t length, const url_alphabet url_alphabet_switch = url_alphabet{false}) {
  if (data == nullptr) return R{};

  if ((length % 4) not_eq 0) {
    throw Decode_error{"Invalid Base64 data"};
  }

  const char* const alphabet = url_alphabet_switch ? base64url_alphabet() : base64_alphabet();
  auto fetch_unit = [data, alphabet](const size_t i) noexcept -> std::array<char, 4> {
    std::array<char, 4> unit;

    unit[0] = std::distance(alphabet, std::find(alphabet, (alphabet + 64), data[i]));
    unit[1] = std::distance(alphabet, std::find(alphabet, (alphabet + 64), data[i + 1]));
    unit[2] = std::distance(alphabet, std::find(alphabet, (alphabet + 64), data[i + 2]));
    unit[3] = std::distance(alphabet, std::find(alphabet, (alphabet + 64), data[i + 3]));

    return unit;
  };

  R buffer;
  buffer.reserve((length * 3) / 4);

  bool has_padding {false};
  if (data[length - 1] == '=') {
    has_padding = true;
    length -= 4;
  }

  size_t i = 0;
  for (;i < length; i += 4) {
    auto&& unit = fetch_unit(i);

    for (int j = 0; j < 4; ++j) {
      if (unit[j] == 64) throw Decode_error{"Invalid Base64 data"};
    }

    buffer.push_back((unit[0] << 2) | (unit[1] >> 4));
    buffer.push_back((unit[1] << 4) | (unit[2] >> 2));
    buffer.push_back((unit[2] << 6) | unit[3]);
  }

  if (has_padding) {
    auto&& unit = fetch_unit(i);
    buffer.push_back((unit[0] << 2) | (unit[1] >> 4));
    if (unit[2] < 64) buffer.push_back((unit[1] << 4) | (unit[2] >> 2));
    if (unit[3] < 64) buffer.push_back((unit[2] << 6) | unit[3]);
  }

  return buffer;
}

/**
 * Decode the specified Base64 encoded data
 *
 * @param data
 *  The data in Base64 format to decode
 *
 * @param url_alphabet_switch
 *   Whether to use the Base64URL alphabet
 *
 * @return The decoded data
 */
template<typename R = std::vector<char>, typename D = std::string>
inline R decode(const D& data, const url_alphabet url_alphabet_switch = url_alphabet{false}) {
  return decode<R>(data.data(), data.size(), url_alphabet_switch);
}

/**
 * Decode a C-String from Base64 format
 *
 * @param data
 *   The C-String to decode from Base64 format
 *
 * @param url_alphabet_switch
 *   Whether to use the Base64URL alphabet
 *
 * @return The decoded data
 */
template<typename R = std::vector<char>>
inline R decode(const char* data, const url_alphabet url_alphabet_switch = url_alphabet{false}) {
  const auto length = std::strlen(data);

  if (length == 0) {
    return R{};
  }

  return decode<R>(data, length, url_alphabet_switch);
}

} //< namespace base64

#endif //< UTIL_BASE64_HPP
