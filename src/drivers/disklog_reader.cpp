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

#include <os>
#include <hw/block_device.hpp>
#include <fs/common.hpp>

#include "disk_logger.hpp"

fs::buffer_t read_log()
{
  auto& device = hw::Devices::drive(DISK_NO);
  const auto sector = disklogger_start_sector(device);
  auto buf = device.read_sync(sector, DISKLOG_SIZE / device.block_size());
  // get header
  const auto* header = (log_structure*) buf->data();
  // create new buffer from log only
  return fs::construct_buffer(header->vla, header->vla + header->length);
}
