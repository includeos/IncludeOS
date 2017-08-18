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

// Slim copy of <api/fs/fat_internal.hpp>

#pragma once
#ifndef FS_FAT_INTERNAL_HPP
#define FS_FAT_INTERNAL_HPP

#include <cstdint>

// Attribute masks
static const uint8_t ATTR_READ_ONLY = 0x01;
static const uint8_t ATTR_HIDDEN    = 0x02;
static const uint8_t ATTR_SYSTEM    = 0x04;
static const uint8_t ATTR_VOLUME_ID = 0x08;
static const uint8_t ATTR_DIRECTORY = 0x10;
static const uint8_t ATTR_ARCHIVE   = 0x20;

// Mask for the last longname entry
static const uint8_t LAST_LONG_ENTRY = 0x40;

struct cl_dir
{
  uint8_t  shortname[11];
  uint8_t  attrib     = 0;
  uint8_t  pad1[8];
  uint16_t cluster_hi = 0;
  uint32_t modified   = 0;
  uint16_t cluster_lo = 0;
  uint32_t filesize   = 0;

} __attribute__((packed));

struct cl_long
{
  uint8_t  index;
  uint16_t first[5];
  uint8_t  attrib;
  uint8_t  entry_type;
  uint8_t  checksum;
  uint16_t second[6];
  uint16_t zero;
  uint16_t third[2];

} __attribute__((packed));

#endif
