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

#include <cstdint>
#include <string>

struct func_offset {
  std::string name;
  uint32_t    offset;
};

struct Elf
{
  // returns the name of a symbol closest to @addr,
  // or the hex representation of addr
  static func_offset
    resolve_symbol(uintptr_t addr);
  static func_offset
    resolve_symbol(void* addr);
  static func_offset
    resolve_symbol(void (*addr)());
  
  //returns the address of a symbol, or 0
  uintptr_t resolve_name(const std::string& name);
};
