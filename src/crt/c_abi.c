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

#include <assert.h>
#include <stdint.h>
#include <kprint>
#define HEAP_ALIGNMENT   63

uint32_t _move_symbols(void* sym_loc)
{
  extern char _ELF_SYM_START_;
  /// read out size of symbols **before** moving them
  extern int  _get_elf_section_datasize(const void*);
  int elfsym_size = _get_elf_section_datasize(&_ELF_SYM_START_);
  elfsym_size = (elfsym_size < HEAP_ALIGNMENT) ? HEAP_ALIGNMENT : elfsym_size;

  /// move ELF symbols to safe area
  extern void _move_elf_syms_location(const void*, void*);
  _move_elf_syms_location(&_ELF_SYM_START_, sym_loc);

  return elfsym_size;
}
