// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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
#ifndef UTIL_CRC32_HPP
#define UTIL_CRC32_HPP

#include <cstdint>
#include <cstddef>

#define CRC32_BEGIN(x)   uint32_t x = 0xFFFFFFFF;
#define CRC32_VALUE(x)   ~(x)

uint32_t crc32(uint32_t partial, const char* buf, size_t len);

inline uint32_t crc32(const void* buf, size_t len)
{
  return ~crc32(0xFFFFFFFF, (const char*) buf, len);
}

#endif
