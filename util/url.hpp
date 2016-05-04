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


/**
 * URL-encode / decode (Percent encoding)
 *
 * Implemented for IncludeOS from RFC 3986
 * https://tools.ietf.org/html/rfc3986#section-2.1
 *
 * @note Encoding reserved chars:
 * The RFC lets the application decide which reserved characters to
 * encode and  which can be used as data. This implementation
 * currently encodes all of them.
 *
 * @note Decoding errors:
 * The RFC only defines what an URL-encoded string is, not what to do
 * if you try to decode a non-encoded (or erroneously encoded)
 * strings. Doing that is therefore considered undefined. By default
 * this implementation will partially decode as much as possible and
 * return that. To throw on error instead, define URL_THROW_ON_ERROR.
 *
 **/

#include <algorithm>
#include <gsl.h>

#ifdef URL_THROW_ON_ERROR
#include <stdexcept>
namespace url {
  class Decode_error : std::runtime_error {
    using runtime_error::runtime_error;
  };

  class Encode_error : std::runtime_error {
    using runtime_error::runtime_error;
  };

  class Hex_error : Decode_error {
    using Decode_error::Decode_error;
  };
}
#endif

namespace url {

  inline std::string decode_error(std::string res) {
#ifdef URL_THROW_ON_ERROR
    throw Decode_error("Decoding incomplete: "+res);
#endif
    return res;
  }

  inline std::string encode_error(std::string res) {
#ifdef URL_THROW_ON_ERROR
    throw Encode_error("Encoding incomplete: "+res);
#endif
    return res;
  }

  inline uint8_t hex_error(char nibble){
#ifdef URL_THROW_ON_ERROR
    throw Hex_error(std::string("Not a hex character: ") + nibble);
#endif
    return 0;
  }


  const char hex[16] =
    {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

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
    static const char reserved[18] =
      {':' , '/' , '?' , '#' , '[' , ']' , '@',
       '!' , '$' , '&' , '\'' , '(' , ')' , '*' , '+' , ',' , ';' , '='};

    return std::find(reserved, reserved + sizeof(reserved), chr)
      < reserved + sizeof(reserved);
  }

  inline bool is_unreserved (char chr) {
    return isalnum(chr) or chr == '-' or chr == '.' or chr == '_' or chr == '~';
  }

  /** URL-encode (percent-encode) a span of bytes */
  inline std::string encode(gsl::span<const char> input){
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

  /** URL-decode (percent-decode) a span of bytes */
  inline std::string decode(gsl::span<const char> input){
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

} // namespace url
