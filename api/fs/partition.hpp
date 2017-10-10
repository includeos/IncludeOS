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
#ifndef FS_PARTITION_HPP
#define FS_PARTITION_HPP

#include <cstdint>

namespace fs
{
  struct Partition {
    explicit Partition(const uint8_t  fl,  const uint8_t  Id,
                       const uint32_t LBA, const uint32_t sz) noexcept
    : flags     {fl},
      id        {Id},
      lba_begin {LBA},
      sectors   {sz}
    {}

    uint8_t  flags;
    uint8_t  id;
    uint32_t lba_begin;
    uint32_t sectors;

    // true if the partition has boot code / is bootable
    bool is_boot() const noexcept
    { return flags & 0x1; }

    // human-readable name of partition id
    std::string name() const;

    // logical block address of beginning of partition
    uint32_t lba() const
    { return lba_begin; }

  }; //< struct Partition

}

#endif
