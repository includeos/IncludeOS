// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef NET_WS_HEADER_HPP
#define NET_WS_HEADER_HPP

#include <cstdint>
#include <cstring>
#include <cassert>

namespace net {

  enum class op_code : uint8_t {
    CONTINUE  = 0,
    TEXT      = 1,
    BINARY    = 2,
    CLOSE     = 8,
    PING      = 9,
    PONG      = 10
  }; // < op_code

  struct ws_header
  {
    uint16_t bits;

    bool is_final() const noexcept {
      return (bits >> 7) & 1;
    }
    void set_final() noexcept {
      bits |= 0x80;
      assert(is_final() == true);
    }
    uint16_t payload() const noexcept {
      return (bits >> 8) & 0x7f;
    }
    void set_payload(const size_t len)
    {
      uint16_t pbits = len;
      if      (len > 65535) pbits = 127;
      else if (len > 125)   pbits = 126;
      bits &= 0x80ff;
      bits |= (pbits & 0x7f) << 8;

      if (is_ext())
          *(uint16_t*) vla = __builtin_bswap16(len);
      else if (is_ext2())
          *(uint64_t*) vla = __builtin_bswap64(len);
      assert(data_length() == len);
    }

    bool is_ext() const noexcept {
      return payload() == 126;
    }
    bool is_ext2() const noexcept {
      return payload() == 127;
    }

    bool is_masked() const noexcept {
      return bits & 0x8000;
    }
    void set_masked(uint32_t mask) noexcept {
      bits |= 0x8000;
      memcpy(keymask(), &mask, sizeof(mask));
    }

    op_code opcode() const noexcept {
      return static_cast<op_code>(bits & 0xf);
    }
    void set_opcode(op_code code) {
      bits &= ~0xf;
      bits |= static_cast<uint8_t>(code) & 0xf;
      assert(opcode() == code);
    }

    bool is_fail() const noexcept {
      return false;
    }

    size_t mask_length() const noexcept {
      return is_masked() ? 4 : 0;
    }
    char* keymask() noexcept {
      return &vla[data_offset() - mask_length()];
    }

    size_t data_offset() const noexcept {
      size_t len = mask_length();
      if (is_ext2()) return len + 8;
      if (is_ext())  return len + 2;
      return len;
    }
    size_t data_length() const noexcept {
      if (is_ext2())
          return __builtin_bswap64(*(uint64_t*) vla) & 0xffffffff;
      if (is_ext())
          return __builtin_bswap16(*(uint16_t*) vla);
      return payload();
    }

    uint16_t header_length() const noexcept
    {
      return sizeof(ws_header) + data_offset();
    }
    static size_t header_length(size_t len, bool client) noexcept
    {
      size_t ofs = sizeof(ws_header) + (client ? 4 : 0);
      if (len > 65535) return ofs + 8;
      if (len > 125)   return ofs + 2;
      return ofs;
    }

    const char* data() const noexcept {
      return &vla[data_offset()];
    }
    char* data() noexcept {
      return &vla[data_offset()];
    }
    void masking_algorithm(char* ptr)
    {
      const char* mask = keymask();
      for (size_t i = 0; i < data_length(); i++)
      {
        ptr[i] = ptr[i] xor mask[i & 3];
      }
    }

    char vla[0];
  } __attribute__((packed)); // < class ws_header

} // < namespace net

#endif
