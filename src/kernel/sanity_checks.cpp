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

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <util/crc32.hpp>

// NOTE: crc_to MUST NOT be initialized to zero
static uint32_t crc_ro = CRC32_BEGIN();
extern char _TEXT_START_;
extern char _TEXT_END_;
extern char _RODATA_START_;
extern char _RODATA_END_;

static uint32_t generate_ro_crc() noexcept
{
  uint32_t crc = CRC32_BEGIN();
  crc = crc32(crc, &_TEXT_START_, &_RODATA_END_ - &_TEXT_START_);
  return CRC32_VALUE(crc);
}

extern "C"
void __init_sanity_checks() noexcept
{
  crc_ro = generate_ro_crc();
}

void kernel_sanity_checks()
{
  assert(crc_ro == generate_ro_crc());
}
