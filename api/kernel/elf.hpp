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
#include <vector>

struct func_offset {
  std::string name;
  uint32_t    addr;
  uint32_t    offset;
};
struct safe_func_offset {
  const char* name;
  uint32_t    addr;
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
  
  static uintptr_t resolve_addr(uintptr_t addr);
  static uintptr_t resolve_addr(void* addr);
  
  // get and resolve the current function
  static func_offset
    get_current_function();
  static std::vector<func_offset>
    get_functions();
  
  // doesn't use heap
  static safe_func_offset
    safe_resolve_symbol(void* addr, char* buffer, size_t length);
  
  //returns the address of a symbol, or 0
  static uintptr_t
    resolve_name(const std::string& name);
  
  // returns the address of the first byte after the ELF header
  // and all static and dynamic sections
  static size_t end_of_file();

  // debugging purposes only
  static const char* get_strtab();
  static void        print_info();
};
