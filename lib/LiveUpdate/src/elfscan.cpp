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
#include <elf.h>
#include <algorithm>
#include <string>
#include <vector>
#include <cassert>

namespace liu
{
  template <typename ElfSym>
  struct SymTab {
    const ElfSym* base = nullptr;
    const size_t  entries = 0;
  };
  struct StrTab {
    const char*  base = nullptr;
    const size_t size = 0;
    StrTab(const char* base, uint32_t size) : base(base), size(size) {}
  };

  template <int Bits, typename ElfSym>
  void* find_sym(SymTab<ElfSym> syms, StrTab strs, const std::string& name)
  {
    for (unsigned i = 0; i < syms.entries; i++)
    {
      const auto& sym = ((const ElfSym*) syms.base)[i];
      int   type;
      if constexpr (Bits == 32)
          type = ELF32_ST_TYPE(sym.st_info);
      else
          type = ELF64_ST_TYPE(sym.st_info);
      // only check functions
      if (type == STT_FUNC) {
        assert(sym.st_name < strs.size);
        const char* sym_name = &strs.base[sym.st_name];
        //printf("Checking %s (%u)\n", sym_name, sym.st_name);
        if (name == sym_name) {
          void* kernel_start = (void*) (uintptr_t) sym.st_value;
          printf("Found %s at %p\n", sym_name, kernel_start);
          return kernel_start;
        }
      }
    }
    return nullptr;
  }

  template <int Bits, typename ElfEhdr, typename ElfShdr, typename ElfSym>
  void* find_kernel_start(const ElfEhdr* hdr_ptr)
  {
    const char* file_ptr = (char*) hdr_ptr;
    const auto* shdr = (ElfShdr*) &file_ptr[hdr_ptr->e_shoff];

#ifndef PLATFORM_UNITTEST
    // check for fast live updates by reading ,nultiboot section
    const auto* fast = (uint32_t*) &file_ptr[shdr[1].sh_offset];
    if (fast[8] == 0xFEE1DEAD) {
      return (void*) (uintptr_t) fast[9];
    }
#endif

    // search for fast_kernel_start by reading symbols
    SymTab<ElfSym> symtab;
    std::vector<StrTab> strtabs;

    for (auto i = 0; i < hdr_ptr->e_shnum; i++)
    {
      switch (shdr[i].sh_type) {
      case SHT_SYMTAB:
        new (&symtab) SymTab<ElfSym> {
              (ElfSym*) &file_ptr[shdr[i].sh_offset],
              (size_t) shdr[i].sh_size / sizeof(ElfSym)
            };
        break;
      case SHT_STRTAB:
        strtabs.emplace_back(&file_ptr[shdr[i].sh_offset], shdr[i].sh_size);
        break;
      default:
        break; // don't care tbh
      }
    }
    if (symtab.base == nullptr || strtabs.empty()) return nullptr;
    StrTab strtab = *std::max_element(std::begin(strtabs), std::end(strtabs),
                    [](auto const& lhs, auto const& rhs) { return lhs.size < rhs.size; });

    // scan symbols and return address of fast_kernel_start, or nullptr
    return find_sym<Bits, ElfSym> (symtab, strtab, "fast_kernel_start");
  }

  void* find_kernel_start32(const Elf32_Ehdr* hdr)
  {
    return find_kernel_start<32, Elf32_Ehdr, Elf32_Shdr, Elf32_Sym> (hdr);
  }
  void* find_kernel_start64(const Elf64_Ehdr* hdr)
  {
    return find_kernel_start<64, Elf64_Ehdr, Elf64_Shdr, Elf64_Sym> (hdr);
  }
}
