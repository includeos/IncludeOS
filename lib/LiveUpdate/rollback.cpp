// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 IncludeOS AS, Oslo, Norway
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
 * Master thesis
 * by Alf-Andre Walla 2016-2017
 *
**/
#include "liveupdate.hpp"
#include <os>

extern uintptr_t heap_end;

namespace liu
{
  static const char* rollback_data;
  static size_t      rollback_len;

void LiveUpdate::rollback_now(const char* reason)
{
  if (LiveUpdate::has_rollback_blob())
  {
    printf("\nPerforming rollback from %p:%u... Reason: %s\n",
	   rollback_data, (uint32_t) rollback_len, reason);
    try
    {
      buffer_t vec(rollback_data, rollback_data + rollback_len);
      // run live update process
      LiveUpdate::exec(vec);
    }
    catch (std::exception& err)
    {
      fprintf(stderr, "Rollback failed:\n%s\n", err.what());
    }
  }
  else {
    fprintf(stderr, "\nMissing rollback data, rebooting...\n");
  }
  fflush(stderr);
  // no matter what, reboot
  os::reboot();
  __builtin_unreachable();
}

const std::pair<const char*, size_t> get_rollback_location()
{
  return {rollback_data, rollback_len};
}

void LiveUpdate::set_rollback_blob(const void* buffer, size_t len) noexcept
{
  rollback_data = (const char*) buffer;
  rollback_len  = len;
  os::on_panic(LiveUpdate::rollback_now);
}
bool LiveUpdate::has_rollback_blob() noexcept
{
                                // minimum legit ELF size
  return rollback_data != nullptr && rollback_len > 164;
}
}

void softreset_service_handler(const void* opaque, size_t length)
{
  // make deep copy?
  auto* data = new char[length];
  memcpy(data, opaque, length);
  liu::rollback_data = data;
  liu::rollback_len  = length;
  os::on_panic(liu::LiveUpdate::rollback_now);
}
