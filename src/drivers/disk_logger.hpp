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

#ifndef DRIVERS_DISK_LOGGER_HPP
#define DRIVERS_DISK_LOGGER_HPP

static const int DISKLOG_SIZE = 16384; // 16kb
static const int DISK_NO = 0;
static_assert(DISKLOG_SIZE % 512 == 0, "Must be a multiple of sector size");

struct log_structure
{
  int64_t  timestamp;
  uint32_t length;
  uint32_t max_length; // don't initialize

  char vla[0];
};

// log is written to the end of the disk image
inline auto disklogger_start_sector(const hw::Block_device& dev)
{
  return dev.size() - DISKLOG_SIZE / dev.block_size();
}

#endif
