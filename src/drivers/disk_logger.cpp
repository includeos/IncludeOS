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
#include <hw/writable_blkdev.hpp>
#include <fs/common.hpp>
#include <rtc>

#include "disk_logger.hpp"

static log_structure header;
static fs::buffer_t  logbuffer;
static uint32_t position = 0;
static bool write_once_when_booted = false;

extern "C" void __serial_print1(const char*);
extern "C" void __serial_print(const char*, size_t);

static void write_all()
{
  try {
    auto& device = (hw::Writable_Block_device&) hw::Devices::drive(DISK_NO);
    const auto sector = disklogger_start_sector(device);
    const bool error = device.write_sync(sector, logbuffer);
    if (error) {
      __serial_print1("IDE::write_sync failed! Missing or wrong driver?\n");
    }
  } catch (std::exception& e) {
    __serial_print1("IDE block device missing! Missing device or driver?\n");
  }
}

static void disk_logger_write(const char* data, size_t len)
{
  if (position + len > header.max_length) {
    position = sizeof(log_structure);
    //header.length = header.max_length;
  }
  __builtin_memcpy(&(*logbuffer)[position], data, len);
  position += len;
  if (header.length < position) header.length = position;

  // update header
  if (OS::is_booted()) {
    header.timestamp = RTC::now();
  }
  else {
    header.timestamp = OS::nanos_since_boot() / 1000000000ull;
  }
  __builtin_memcpy(logbuffer->data(), &header, sizeof(log_structure));

  // write to disk when we are able
  const bool once = OS::is_booted() && write_once_when_booted == false;
  if (OS::block_drivers_ready() && (once || OS::is_panicking()))
  {
    write_once_when_booted = true;
    write_all();
  }
}

__attribute__((constructor))
static void enable_disk_logger()
{
  logbuffer = fs::construct_buffer(DISKLOG_SIZE);
  position = sizeof(log_structure);
  header.max_length = logbuffer->capacity();
  OS::add_stdout(disk_logger_write);
}
