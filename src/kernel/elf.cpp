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

#include <cassert>
#include <string>
#include <debug>
#include "../../vmbuild/elf.h"

static const uintptr_t ELF_START = 0x200000;

extern "C" char *
__cxa_demangle(const char *name, char *buf, size_t *n, int *status);

class ElfTables
{
public:
  ElfTables(uintptr_t elf_base)
    : ELF_BASE(elf_base)
  {
    auto& elf_hdr = *(Elf32_Ehdr*) ELF_BASE;
    
    // enumerate all section headers
    auto* shdr = (Elf32_Shdr*) (ELF_BASE + elf_hdr.e_shoff);
    for (Elf32_Half i = 0; i < elf_hdr.e_shnum; i++)
    {
      switch (shdr[i].sh_type)
      {
      case SHT_SYMTAB:
        this->symtab = (Elf32_Sym*) (ELF_BASE + shdr[i].sh_offset);
        this->st_entries = shdr[i].sh_size / sizeof(Elf32_Sym);
        debug("found symbol table at %p with %u entries\n", 
            this->symtab, this->st_entries);
        break;
      case SHT_STRTAB:
        this->strtab = (char*) (ELF_BASE + shdr[i].sh_offset);
        debug("found string table at %p\n", this->strtab);
        break;
      case SHT_DYNSYM:
      default:
        // don't care tbh
        break;
      }
    }
    assert(symtab && strtab);
  }
  
  std::string getsym(Elf32_Addr addr)
  {
    for (size_t i = 0; i < st_entries; i++) {
      // find entry with matching address
      if (symtab[i].st_value == addr) {
        // return string name for symbol
        return demangle( sym_name(symtab[i]) );
      }
    }
    return "(missing symbol)";
  }
  inline std::string getsym(void(*func)()) {
    return getsym((Elf32_Addr) func);
  }
  
  
private:
  const char* sym_name(Elf32_Sym& sym) const {
    return &strtab[sym.st_name];
  }
  bool is_func(Elf32_Sym* sym) const
  {
    return ELF32_ST_TYPE(sym->st_info) == STT_FUNC;
  }
  std::string demangle(const char* name)
  {
    // try demangle the name
    std::string buf;
    buf.reserve(64);
    size_t buflen = buf.capacity();
    int status;
    // internally, demangle just returns buf when status is ok
    __cxa_demangle(name, (char*) buf.data(), &buflen, &status);
    if (status) return name;
    return buf;
  }


  Elf32_Sym* symtab = 0x0;
  size_t     st_entries = 0;
  char*      strtab = 0x0;
  uintptr_t  ELF_BASE;
};

ElfTables& get_parser() {
  static ElfTables parser(ELF_START);
  return parser;
}

std::string resolve_symbol(uintptr_t addr)
{
  return get_parser().getsym(addr);
}
