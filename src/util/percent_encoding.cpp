// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#include <algorithm>
#include <array>

#include <util/percent_encoding.hpp>

///////////////////////////////////////////////////////////////////////////////
static inline std::string decode_error(std::string res) {
  auto error_message = "Decoding incomplete: " + res;
#ifdef URI_THROW_ON_ERROR
  throw uri::Decode_error{error_message};
#endif
  return error_message;
}

///////////////////////////////////////////////////////////////////////////////
static inline uint8_t hex_error(const char nibble) {
#ifdef URI_THROW_ON_ERROR
  throw uri::Hex_error{std::string{"Not a hex character: "} + nibble};
#endif
  (void) nibble;
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
static inline uint8_t from_hex(const char digit) {
  if (digit >= '0' and digit <= '9')
      return digit - '0';

  if (digit >= 'a' and digit <= 'f')
    return digit - 'a' + 10;

  if (digit >= 'A' and digit <= 'F')
    return digit - 'A' + 10;

  return hex_error(digit);
}

///////////////////////////////////////////////////////////////////////////////
static inline uint8_t from_hex(const char nibble1, const char nibble2) {
  return (from_hex(nibble1) << 4) + from_hex(nibble2);
}

///////////////////////////////////////////////////////////////////////////////
static inline bool is_reserved (const char chr) noexcept {
  static const std::array<char,18> reserved
  {{
    ':' , '/' , '?' , '#' , '[' , ']' , '@', '!' , '$' ,
    '&' , '\'' , '(' , ')' , '*' , '+' , ',' , ';' , '='
  }};

  return std::find(reserved.begin(), reserved.end(), chr) not_eq reserved.end();
}

///////////////////////////////////////////////////////////////////////////////
static inline bool is_unreserved (const char chr) noexcept {
  return std::isalnum(chr) or (chr == '-') or (chr == '.') or (chr == '_') or (chr == '~');
}

///////////////////////////////////////////////////////////////////////////////
std::string uri::encode(util::csview input) {
  static const std::array<char,16> hex
  {{ '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' }};

  std::string res;
  res.reserve(input.size() * 3);

  for (const auto chr : input)
    if (is_unreserved(chr)) {
      res += chr;
    } else {
      res += '%';
      res += hex[ chr >> 4  ];
      res += hex[ chr & 0xf ];
    }

  res.shrink_to_fit();
  return res;
}

///////////////////////////////////////////////////////////////////////////////
std::string uri::decode(util::csview input) {
  std::string res;
  res.reserve(input.size());

  for (auto it = input.cbegin(), e = input.cend(); it not_eq e; ++it) {
    if (*it == '%') {

      if (++it >= e) return decode_error(std::move(res));
      const uint8_t nibble1 = (*it);

      if (++it >= e) return decode_error(std::move(res));
      const uint8_t nibble2 = (*it);

      res += static_cast<char>(from_hex(nibble1, nibble2));

    } else {
      if (is_reserved(*it) or is_unreserved(*it))
        res += *it;
      else
        return decode_error(std::move(res));
    }
  }

  res.shrink_to_fit();
  return res;
}
