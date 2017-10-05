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

#ifndef MENDER_COMMON_HPP
#define MENDER_COMMON_HPP

#ifndef RAPIDJSON_HAS_STDSTRING
  #define RAPIDJSON_HAS_STDSTRING 1
#endif

#include <fs/disk.hpp>
#include <botan/secmem.h>

//#define VERBOSE_MENDER 1
// Informational prints
#ifdef VERBOSE_MENDER
  #define MENDER_INFO(FROM, TEXT, ...) printf("%13s ] " TEXT "\n", "[ " FROM, ##__VA_ARGS__)
  #define MENDER_INFO2(TEXT, ...) printf("%16s" TEXT "\n"," ", ##__VA_ARGS__)
#else
  #define MENDER_INFO(X,...)
  #define MENDER_INFO2(X,...)
#endif

namespace mender {

  using Storage   = std::shared_ptr<fs::Disk>;

  /** Byte sequence */
  using byte_seq  = std::vector<uint8_t>;
  //using byte_seq = Botan::secure_vector<Botan::byte>;
}

#endif
