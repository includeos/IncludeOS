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
#include <string>
#include <uri>

inline std::string decode_error(std::string res) {
#ifdef URL_THROW_ON_ERROR
  throw uri::Decode_error("Decoding incomplete: "+res);
#endif
  return res;
}

inline std::string encode_error(std::string res) {
#ifdef URL_THROW_ON_ERROR
  throw uri::Encode_error("Encoding incomplete: "+res);
#endif
  return res;
}

inline uint8_t hex_error(char nibble){
#ifdef URL_THROW_ON_ERROR
  throw uri::Hex_error(std::string("Not a hex character: ") + nibble);
#else
  (void) nibble;
#endif
  return 0;
}

inline uint8_t from_hex(char digit){
  if (digit >= '0' and digit <= '9')
      return digit - '0';

  if (digit >= 'a' and digit <= 'f')
    return digit - 'a' + 10;

  if (digit >= 'A' and digit <= 'F')
    return digit - 'A' + 10;

  return hex_error(digit);
}

inline uint8_t from_hex(char nibble1, char nibble2) {
  return (from_hex(nibble1) << 4) + from_hex(nibble2);
}

inline bool is_reserved (char chr) {
  static const std::array<char,18> reserved
  {{':' , '/' , '?' , '#' , '[' , ']' , '@',
        '!' , '$' , '&' , '\'' , '(' , ')' , '*' , '+' , ',' , ';' , '='}};

  return std::find(reserved.begin(), reserved.end(), chr) < reserved.end();
}

inline bool is_unreserved (char chr) {
  return isalnum(chr) or chr == '-' or chr == '.' or chr == '_' or chr == '~';
}

std::string uri::encode(gsl::span<const char> input){
  static const std::array<char,16> hex =
    {{ '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' }};

  std::string res;

  for (unsigned char chr : input)
    if (is_unreserved(chr)) {
      res += chr;
    } else {
      res += '%';
      res += hex[ chr >> 4 ];
      res += hex[ chr & 0xf ];
    }

  return res;
}

std::string uri::decode(gsl::span<const char> input){
  std::string res ="";
  for (auto it = input.begin(); it != input.end(); it++) {
    if (*it == '%') {

      if (++it >= input.end()) return decode_error(res);
      uint8_t nibble1 = *(it);

      if (++it >= input.end()) return decode_error(res);
      uint8_t nibble2 = *(it);

      res += (char)from_hex(nibble1,nibble2);

    } else {
      if (is_reserved(*it) or is_unreserved(*it))
        res += *it;
      else
        return decode_error(res);
    }
  }
  return res;
}
