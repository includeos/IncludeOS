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

#include <kernel/elf.hpp>
#include <cassert>
#include <cstdio>
#include <string>
#include <debug>
#include <vector>
#include "../../vmbuild/elf.h"

static const uintptr_t ELF_START = 0x200000;

extern "C" char *
__cxa_demangle(const char *name, char *buf, size_t *n, int *status);

template <typename N>
std::string to_hex_string(N n)
{
  std::string buffer; buffer.reserve(64);
  snprintf((char*) buffer.data(), buffer.capacity(), "%#x", n);
  return buffer;
}

struct SymTab {
  Elf32_Sym* base;
  uint32_t   entries;
};

class ElfTables
{
public:
  ElfTables(uintptr_t elf_base)
    : strtab(0), ELF_BASE(elf_base)
  {
    auto& elf_hdr = *(Elf32_Ehdr*) ELF_BASE;
    
    // enumerate all section headers
    auto* shdr = (Elf32_Shdr*) (ELF_BASE + elf_hdr.e_shoff);
    for (Elf32_Half i = 0; i < elf_hdr.e_shnum; i++)
    {
      switch (shdr[i].sh_type)
      {
      case SHT_SYMTAB:
        symtab.push_back({ (Elf32_Sym*) (ELF_BASE + shdr[i].sh_offset) ,
                           shdr[i].sh_size / sizeof(Elf32_Sym) });
        //printf("found symtab at %#x\n", shdr[i].sh_offset);
        //debug("found symbol table at %p with %u entries\n", 
        //    this->symtab, this->st_entries);
        break;
      case SHT_STRTAB:
        this->strtab = (char*) (ELF_BASE + shdr[i].sh_offset);
        break;
      case SHT_DYNSYM:
      default:
        // don't care tbh
        break;
      }
    }
    assert(!symtab.empty() && strtab);
  }
  
  func_offset getsym(Elf32_Addr addr)
  {
    // probably just a null pointer with ofs=addr
    if (addr < 0x7c00) return {"(null) + " + to_hex_string(addr), addr};
    // definitely in the bootloader
    if (addr < 0x7e00) return {"Bootloader area", addr - 0x7c00};
    // resolve manually from symtab
    for (auto& tab : symtab)
    for (size_t i = 0; i < tab.entries; i++) {
      // find entry with matching address
      if (addr >= tab.base[i].st_value
      && (addr < tab.base[i].st_value + tab.base[i].st_size)) {
        
        auto offset = addr - tab.base[i].st_value;
        // return string name for symbol
        return {demangle( sym_name(tab.base[i]) ), offset};
      }
    }
    return {to_hex_string(addr), 0};
  }
  Elf32_Addr getaddr(const std::string& name)
  {
    for (auto& tab : symtab)
    for (size_t i = 0; i < tab.entries; i++) {
      // find entry with matching address
      if (sym_name(tab.base[i]) == name)
        return tab.base[i].st_value;
    }
    return 0;
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
    size_t buflen = 128;
    std::string buf;
    buf.reserve(buflen);
    int status;
    // internally, demangle just returns buf when status is ok
    auto res = __cxa_demangle(name, (char*) buf.data(), &buflen, &status);
    if (status) return std::string(name);
    return std::string(res);
  }

  std::vector<SymTab> symtab;
  const char* strtab;
  uintptr_t   ELF_BASE;
};

ElfTables& get_parser() {
  static ElfTables parser(ELF_START);
  return parser;
}

func_offset Elf::resolve_symbol(uintptr_t addr)
{
  return get_parser().getsym(addr);
}
func_offset Elf::resolve_symbol(void* addr)
{
  return get_parser().getsym((uintptr_t) addr);
}
func_offset Elf::resolve_symbol(void (*addr)())
{
  return get_parser().getsym((uintptr_t) addr);
}

void print_backtrace()
{
  #define frp(N, ra)                                 \
    (__builtin_frame_address(N) != nullptr) &&       \
      (ra = __builtin_return_address(N)) != nullptr

  #define PRINT_TRACE(N, ra)                      \
    auto symb = Elf::resolve_symbol(ra);          \
    printf("[%d] %8p + 0x%.4x: %s\n",             \
        N, ra, symb.offset, symb.name.c_str());

  printf("\n");
  void* ra;
  if (frp(0, ra)) {
    PRINT_TRACE(0, ra);
    if (frp(1, ra)) {
      PRINT_TRACE(1, ra);
      if (frp(2, ra)) {
        PRINT_TRACE(2, ra);
        if (frp(3, ra)) {
          PRINT_TRACE(3, ra);
          if (frp(4, ra)) {
            PRINT_TRACE(4, ra);
            if (frp(5, ra)) {
              PRINT_TRACE(5, ra);
              if (frp(6, ra)) {
                PRINT_TRACE(6, ra);
            }}}}}}}
}
