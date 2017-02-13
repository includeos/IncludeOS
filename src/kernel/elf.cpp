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
#include <util/crc32.hpp>
#include <common>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <vector>
#include "../../vmbuild/elf.h"

static const char* null_stringz = "(null)";
static const char* boot_stringz = "Bootloader area";
extern "C" char _ELF_START_;
static const uintptr_t ELF_START = reinterpret_cast<uintptr_t>(&_ELF_START_);

#define frp(N, ra)                                 \
  (__builtin_frame_address(N) != nullptr) &&       \
    (ra = __builtin_return_address(N)) != nullptr

extern "C" char *
__cxa_demangle(const char *name, char *buf, size_t *n, int *status);

template <typename N>
static std::string to_hex_string(N n)
{
  char buffer[16];
  int len = snprintf(buffer, sizeof(buffer), "%#x", n);
  return std::string(buffer, len);
}

static Elf32_Ehdr& elf_header() {
  return *(Elf32_Ehdr*) ELF_START;
}

struct SymTab {
  Elf32_Sym* base;
  uint32_t   entries;
};
struct StrTab {
  const char* base;
  uint32_t    size;
};

class ElfTables
{
public:
  ElfTables() {}

  void set(Elf32_Sym* syms, 
           uint32_t   entries, 
           const char* string_table,
           uint32_t    strsize,
           uint32_t csum_syms,
           uint32_t csum_strs)
  {
    symtab    = {(Elf32_Sym*) syms, entries};
    strtab    = {string_table, strsize};
    checksum_syms = csum_syms;
    checksum_strs = csum_strs;
  }

  func_offset getsym(Elf32_Addr addr)
  {
    // probably just a null pointer with ofs=addr
    if (UNLIKELY(addr < 0x1000))
        return {null_stringz, 0, addr};
    // definitely in the bootloader
    if (UNLIKELY(addr >= 0x7c00 && addr < 0x7e00))
        return {boot_stringz, 0x7c00, addr - 0x7c00};
    // resolve manually from symtab
    auto* sym = getaddr(addr);
    // validate symbol address
    //assert(sym >= symtab.base && sym < symtab.base + symtab.entries);
    if (LIKELY(sym)) {
      auto base   = sym->st_value;
      auto offset = addr - base;
      // return string name for symbol
      const char* name = sym_name(sym);
      if (name)
          return {demangle(name), base, offset};
      else
          return {to_hex_string(base), base, offset};
    }
    // function or space not found
    return {to_hex_string(addr), addr, 0};
  }
  safe_func_offset getsym_safe(Elf32_Addr addr, char* buffer, size_t length)
  {
    // probably just a null pointer with ofs=addr
    if (UNLIKELY(addr < 0x1000))
        return {null_stringz, 0, addr};
    // definitely in the bootloader
    if (UNLIKELY(addr >= 0x7c00 && addr < 0x7e00))
        return {boot_stringz, 0x7c00, addr - 0x7c00};
    // resolve manually from symtab
    auto* sym = getaddr(addr);
    if (LIKELY(sym)) {
      auto base   = sym->st_value;
      auto offset = addr - base;
      // return string name for symbol
      return {demangle_safe( sym_name(sym), buffer, length ), base, offset};
    }
    // function or space not found
    snprintf(buffer, length, "0x%08x", addr);
    return {buffer, addr, 0};
  }

  Elf32_Addr getaddr(const std::string& name)
  {
    for (size_t i = 0; i < symtab.entries; i++)
    {
      auto& sym = symtab.base[i];
      //if (ELF32_ST_TYPE(sym.st_info) == STT_FUNC)
      if (demangle( sym_name(&sym) ) == name)
          return sym.st_value;
    }
    return 0;
  }
  Elf32_Sym* getaddr(Elf32_Addr addr)
  {
    for (size_t i = 0; i < symtab.entries; i++) {

      //if (ELF32_ST_TYPE(symtab.base[i].st_info) == STT_FUNC)
      if (addr >= symtab.base[i].st_value
      && (addr <  symtab.base[i].st_value + symtab.base[i].st_size))
          return &symtab.base[i];
    }
    return nullptr;
  }

  size_t end_of_file() const {
    auto& hdr = elf_header();
    return hdr.e_ehsize + (hdr.e_phnum * hdr.e_phentsize) + (hdr.e_shnum * hdr.e_shentsize);
  }

  const SymTab& get_symtab() const {
    return symtab;
  }

  const auto* get_strtab() const {
    return strtab.base;
  }

  bool verify_symbols() const {
    uint32_t csum = 
        crc32(symtab.base, symtab.entries * sizeof(Elf32_Sym));
    if (csum != checksum_syms) {
      printf("ELF symbol tables checksum failed! "
              "(%08x vs %08x)\n", csum, checksum_syms);
      return false;
    }
    csum = crc32(strtab.base, strtab.size);
    if (csum != checksum_strs) {
      printf("ELF string tables checksum failed! "
              "(%08x vs %08x)\n", csum, checksum_strs);
      return false;
    }
    return true;
  }

private:
  const char* sym_name(Elf32_Sym* sym) const {
    return &strtab.base[sym->st_name];
  }
  std::string demangle(const char* name) const
  {
    char buffer[2048];
    const char* res = demangle_safe(name, buffer, sizeof(buffer));
    if (res)
        return std::string(res);
    else
        return std::string(name);
  }
  const char* demangle_safe(const char* name, char* buffer, size_t buflen) const
  {
    int status;
    // internally, demangle just returns buf when status is ok
    auto* res = __cxa_demangle(name, (char*) buffer, &buflen, &status);
    if (status) return name;
    return res;
  }

  SymTab    symtab;
  StrTab    strtab;
  uint32_t  checksum_syms;
  uint32_t  checksum_strs;
};
static ElfTables parser;

ElfTables& get_parser() {
  return parser;
}

size_t Elf::end_of_file() {
  return get_parser().end_of_file();
}
const char* Elf::get_strtab() {
  return get_parser().get_strtab();
}

func_offset Elf::resolve_symbol(uintptr_t addr)
{
  return get_parser().getsym(addr);
}
func_offset Elf::resolve_symbol(void* addr)
{
  return get_parser().getsym((uintptr_t) addr);
}

uintptr_t Elf::resolve_addr(uintptr_t addr)
{
  auto* sym = get_parser().getaddr(addr);
  if (sym) return sym->st_value;
  return addr;
}
uintptr_t Elf::resolve_addr(void* addr)
{
  auto* sym = get_parser().getaddr((uintptr_t) addr);
  if (sym) return sym->st_value;
  return (uintptr_t) addr;
}

safe_func_offset Elf::safe_resolve_symbol(void* addr, char* buffer, size_t length)
{
  return get_parser().getsym_safe((Elf32_Addr) addr, buffer, length);
}
uintptr_t Elf::resolve_name(const std::string& name)
{
  return get_parser().getaddr(name);
}

func_offset Elf::get_current_function()
{
  return resolve_symbol(__builtin_return_address(0));
}
std::vector<func_offset> Elf::get_functions()
{
  std::vector<func_offset> vec;
  #define ADD_TRACE(N, ra)                      \
      vec.push_back(Elf::resolve_symbol(ra));

  void* ra;
  if (frp(0, ra)) {
    ADD_TRACE(0, ra);
    if (frp(1, ra)) {
      ADD_TRACE(1, ra);
      if (frp(2, ra)) {
        ADD_TRACE(2, ra);
        if (frp(3, ra)) {
          ADD_TRACE(3, ra);
          if (frp(4, ra)) {
            ADD_TRACE(4, ra);
            if (frp(5, ra)) {
              ADD_TRACE(5, ra);
              if (frp(6, ra)) {
                ADD_TRACE(6, ra);
                if (frp(7, ra)) {
                  ADD_TRACE(7, ra);
                  if (frp(8, ra)) {
                    ADD_TRACE(8, ra);
  }}}}}}}}}
  return vec;
}

bool Elf::verify_symbols()
{
  return get_parser().verify_symbols();
}

void print_backtrace()
{
  char _symbol_buffer[1024];
  char _btrace_buffer[1024];

  if (Elf::get_strtab() == NULL) {
    int len = snprintf(_btrace_buffer, sizeof(_btrace_buffer),
              "symtab or strtab is empty, indicating image may be stripped\n");
    write(1, _btrace_buffer, len);
  }

  #define PRINT_TRACE(N, ra) \
    auto symb = Elf::safe_resolve_symbol(                     \
                ra, _symbol_buffer, sizeof(_symbol_buffer));  \
    int len = snprintf(_btrace_buffer, sizeof(_btrace_buffer),\
             "[%d] %8x + 0x%.3x: %s\n", \
             N, symb.addr, symb.offset, symb.name);\
    write(1, _btrace_buffer, len);

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
                if (frp(7, ra)) {
                  PRINT_TRACE(7, ra);
                  if (frp(8, ra)) {
                    PRINT_TRACE(8, ra);
  }}}}}}}}}
}

void Elf::print_info()
{
  auto& hdr = elf_header();
  // program headers
  auto* phdr = (Elf32_Phdr*) (ELF_START + hdr.e_phoff);
  printf("program headers offs=%#x at phys %p\n", hdr.e_phoff, phdr);
  // section headers
  auto* shdr = (Elf32_Shdr*) (ELF_START + hdr.e_shoff);
  printf("section headers offs=%#x at phys %p\n", hdr.e_shoff, shdr);
  for (Elf32_Half i = 0; i < hdr.e_shnum; i++)
  {
    uintptr_t start = ELF_START + shdr[i].sh_offset;
    uintptr_t end   = start     + shdr[i].sh_size;
    printf("sh from %#x to %#x\n", start, end);
  }

}

#include <kprint>
extern "C"
void _print_elf_symbols()
{
  const auto& symtab = parser.get_symtab();
  const char* strtab = parser.get_strtab();

  for (size_t i = 0; i < symtab.entries; i++)
  {
    kprintf("%8x: %s\n", symtab.base[i].st_value, &strtab[symtab.base[i].st_name]);
  }
  kprintf("*** %u entries\n", symtab.entries);
}
extern "C"
void _validate_elf_symbols()
{
  const auto& symtab = parser.get_symtab();
  const char* strtab = parser.get_strtab();
  if (symtab.entries == 0 || strtab == nullptr) return;

  for (size_t i = 1; i < symtab.entries; i++)
  {
    if (symtab.base[i].st_value != 0) {
      assert(symtab.base[i].st_value > 0x2000);
      const char* string = &strtab[symtab.base[i].st_name];
      assert(strlen(string));
    }
  }
}

static struct relocated_header {
  Elf32_Sym* syms = nullptr;
  uint32_t   entries = 0xFFFF;
  uint32_t   strsize = 0xFFFF;
  uint32_t   check_syms = 0xFFFF;
  uint32_t   check_strs = 0xFFFF;

  const char* strings() const {
    return (char*) &syms[entries];
  }
} relocs;

static const char* SANITY_STRING = "Hello world!";
struct elfsyms_header {
  uint32_t  symtab_entries;
  uint32_t  strtab_size;
  uint32_t  sanity_check;
  uint32_t  checksum_syms;
  uint32_t  checksum_strs;
  Elf32_Sym syms[0];
} __attribute__((packed));

extern "C"
int _get_elf_section_datasize(const void* location)
{
  auto& hdr = *(elfsyms_header*) location;
  // special case for missing call to elf_syms
  if (hdr.symtab_entries == 0) return 0;
  return hdr.symtab_entries * sizeof(Elf32_Sym) + hdr.strtab_size;
}

extern "C"
void _move_elf_syms_location(const void* location, void* new_location)
{
  int size = _get_elf_section_datasize(location);
  // stripped variant
  if (size == 0) {
    relocs.entries = 0;
    relocs.strsize = 0;
    return;
  }
  // incoming header
  auto* hdr = (elfsyms_header*) location;
  // verify CRC sanity check
  const uint32_t our_sanity = crc32(SANITY_STRING, strlen(SANITY_STRING));
  if (hdr->sanity_check != our_sanity)
  {
    kprintf("CRC sanity check failed! "
            "(%08x vs %08x)\n", hdr->sanity_check, our_sanity);
    relocs.entries = 0;
    relocs.strsize = 0;
    return;
  }

  // verify separate checksums of symbols and strings
  uint32_t symbsize = hdr->symtab_entries * sizeof(Elf32_Sym);
  uint32_t csum_syms = crc32((char*) hdr->syms, symbsize);
  uint32_t csum_strs = crc32((char*) &hdr->syms[hdr->symtab_entries], hdr->strtab_size);
  if (csum_syms != hdr->checksum_syms || csum_strs != hdr->checksum_strs)
  {
    if (csum_syms != hdr->checksum_syms)
      kprintf("ELF symbol tables checksum failed! "
              "(%08x vs %08x)\n", csum_syms, hdr->checksum_syms);
    if (csum_strs != hdr->checksum_strs)
      kprintf("ELF string tables checksum failed! "
              "(%08x vs %08x)\n", csum_strs, hdr->checksum_strs);
    uint32_t all = crc32(hdr, sizeof(elfsyms_header) + size);
    kprintf("Checksum ELF section: %08x\n", all);

    relocs.entries = 0;
    relocs.strsize = 0;
    return;
  }
  // update header
  relocs.syms    = (Elf32_Sym*) new_location;
  relocs.entries = hdr->symtab_entries;
  relocs.strsize = hdr->strtab_size;
  relocs.check_syms = hdr->checksum_syms;
  relocs.check_strs = hdr->checksum_strs;
  // copy **overlapping** symbol and string data
  memmove((char*) relocs.syms, (char*) hdr->syms, size);
}

extern "C"
void _init_elf_parser()
{
  if (relocs.entries) {
    // apply changes to the symbol parser from custom location
    parser.set(relocs.syms,      relocs.entries, 
               relocs.strings(), relocs.strsize,
               relocs.check_syms, relocs.check_strs);
  }
  else {
    // symbols and strings are stripped out
    parser.set(nullptr, 0, nullptr, 0,  0, 0);
  }
}
