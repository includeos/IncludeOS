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
#ifndef UTIL_ASYNC_HPP
#define UTIL_ASYNC_HPP

#include <net/inet>
#include <net/stream.hpp>
#include <fs/disk.hpp>

class Async
{
public:
  static const size_t PAYLOAD_SIZE = 64000;

  using Stream  = net::Stream;
  using Disk    = fs::Disk_ptr;
  using Dirent  = fs::Dirent;

  typedef delegate<void(bool)> next_func;
  typedef delegate<void(fs::error_t, bool)> on_after_func;
  typedef delegate<void(fs::buffer_t, next_func)> on_write_func;

  static void upload_file(
      Disk,
      const Dirent&,
      Stream*,
      on_after_func,
      size_t = PAYLOAD_SIZE);

  static void disk_transfer(
      Disk,
      const Dirent&,
      on_write_func,
      on_after_func,
      size_t = PAYLOAD_SIZE);

};

#endif
