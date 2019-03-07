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
#include <elf.h>
#include <os.hpp>
#include <arch.hpp>

#include <stdint.h>
#if UINTPTR_MAX == 0xffffffffffffffff
  typedef Elf64_Sym   ElfSym;
  typedef Elf64_Ehdr  ElfEhdr;
  typedef Elf64_Phdr  ElfPhdr;
  typedef Elf64_Shdr  ElfShdr;
  typedef Elf64_Addr  ElfAddr;
#elif UINTPTR_MAX == 0xffffffff
  typedef Elf32_Sym   ElfSym;
  typedef Elf32_Ehdr  ElfEhdr;
  typedef Elf32_Phdr  ElfPhdr;
  typedef Elf32_Shdr  ElfShdr;
  typedef Elf32_Addr  ElfAddr;
#else
  #error "Unknown data model"
#endif

static const char* boot_stringz = "Bootloader area";
extern "C" char _ELF_START_;
static const uintptr_t ELF_START = reinterpret_cast<uintptr_t>(&_ELF_START_);

#define frp(N, ra)                                 \
  (__builtin_frame_address(N) != nullptr) &&       \
  (ra = __builtin_return_address(N)) != nullptr && ra != (void*)-1 \

extern "C" char *
__cxa_demangle(const char *name, char *buf, size_t *n, int *status);

template <typename N>
static std::string to_hex_string(N n)
{
  char buffer[16];
  int len = snprintf(buffer, sizeof(buffer), "%#x", n);
  return std::string(buffer, len);
}

static ElfEhdr& elf_header() {
  return *(ElfEhdr*) ELF_START;
}

struct SymTab {
  const ElfSym* base;
  uint32_t      entries;
};
struct StrTab {
  const char* base;
  uint32_t    size;
};

class ElfTables
{
public:
  ElfTables() {}

  void set(const ElfSym* syms,
           uint32_t    entries,
           const char* strs,
           uint32_t    strsize,
           uint32_t csum_syms,
           uint32_t csum_strs)
  {
    /*
    auto* symbase = new ElfSym[entries];
    std::copy(syms, syms + entries, symbase);
    char* strbase = new char[strsize];
    std::copy(string_table, string_table + strsize, strbase);
    */
    this->symtab = {syms, entries};
    this->strtab = {strs, strsize};
    this->checksum_syms = csum_syms;
    this->checksum_strs = csum_strs;
  }

  safe_func_offset getsym_safe(ElfAddr addr, char* buffer, size_t length)
  {
    // inside bootloader area
    if (UNLIKELY(addr >= 0x7c00 && addr < 0x7e00))
        return {boot_stringz, 0x7c00, (uint32_t) addr - 0x7c00};
    // find symbol name for all addresses above first page
    // which are treated as possible null pointers
    if (LIKELY(addr > 0x1000))
    {
      // resolve manually from symtab
      const auto* sym = getaddr(addr);
      if (LIKELY(sym)) {
        auto     base   = sym->st_value;
        uint32_t offset = (uint32_t) (addr - base);
        // return string name for symbol
        return {demangle_safe( sym_name(sym), buffer, length ), static_cast<uintptr_t>(base), offset};
      }
    }
    else if (addr == 0x0) {
      return {"0x0 (null)", static_cast<uintptr_t>(addr), 0};
    }
    // function or space not found
    snprintf(buffer, length, "%p", (void*) addr);
    return {buffer, static_cast<uintptr_t>(addr), 0};
  }

  const ElfSym* getaddr(ElfAddr addr)
  {
    // find exact match
    for (int i = 0; i < (int) symtab.entries; i++)
    {
      if (addr >= symtab.base[i].st_value
      && (addr <  symtab.base[i].st_value + symtab.base[i].st_size)) {
          //printf("found sym (%p) at %d\n", (void*) addr, i);
          return &symtab.base[i];
        }
    }
    // try again, but use closest match
    const ElfSym* guess = nullptr;
    uintptr_t     gdiff = 512;
    for (size_t i = 0; i < symtab.entries; i++)
    {
      if (addr >= symtab.base[i].st_value
      && (addr <  symtab.base[i].st_value + 512))
      {
        auto diff = addr - symtab.base[i].st_value;
        if (diff < gdiff) {
          gdiff = diff;
          guess = &symtab.base[i];
        }
      }
    }
    return guess;
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
    //printf("ELF verification from %p to %p (Syms=%#x, Strs=%#x)\n",
    //        symtab.base, strtab.base + strtab.size, checksum_syms, checksum_strs);
    uint32_t csum =
        crc32_fast(symtab.base, symtab.entries * sizeof(ElfSym));
    if (csum != checksum_syms) {
      printf("ELF symbol tables checksum failed! "
              "(%08x vs %08x)\n", csum, checksum_syms);
      return false;
    }
    csum = crc32_fast(strtab.base, strtab.size);
    if (csum != checksum_strs) {
      printf("ELF string tables checksum failed! "
              "(%08x vs %08x)\n", csum, checksum_strs);
      return false;
    }
    return true;
  }

private:
  const char* sym_name(const ElfSym* sym) const {
    return &strtab.base[sym->st_name];
  }
  const char* demangle_safe(const char* name, char* buffer, size_t buflen) const
  {
    int status;
    // internally, demangle just returns buf when status is ok
    auto* res = __cxa_demangle(name, buffer, &buflen, &status);
    if (status == 0) return res;
    return name;
  }

  SymTab    symtab;
  StrTab    strtab;
  /* NOTE: DON'T INITIALIZE */
  uint32_t  checksum_syms;
  uint32_t  checksum_strs;
  /* NOTE: DON'T INITIALIZE */
  friend void elf_protect_symbol_areas();
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
  return get_parser().getsym_safe((ElfAddr) addr, buffer, length);
}

bool Elf::verify_symbols()
{
  return get_parser().verify_symbols();
}

void os::print_backtrace(void(*stdout_function)(const char*, size_t)) noexcept
{
  char _symbol_buffer[8192];
  char _btrace_buffer[8192];

  if (Elf::get_strtab() == NULL) {
    int len = snprintf(_btrace_buffer, sizeof(_btrace_buffer),
              "symtab or strtab is empty, indicating image may be stripped\n");
    write(1, _btrace_buffer, len);
  }

#if UINTPTR_MAX == 0xffffffff
  #define PRINT_TRACE(N, ra) \
    auto symb = Elf::safe_resolve_symbol(                     \
                ra, _symbol_buffer, sizeof(_symbol_buffer));  \
    int len = snprintf(_btrace_buffer, sizeof(_btrace_buffer),\
            "[%d] 0x%08x + 0x%.3x: %s\n",         \
            N, symb.addr, symb.offset, symb.name);\
            stdout_function(_btrace_buffer, len);

#elif UINTPTR_MAX == 0xffffffffffffffff
  #define PRINT_TRACE(N, ra) \
    auto symb = Elf::safe_resolve_symbol(                     \
                ra, _symbol_buffer, sizeof(_symbol_buffer));  \
    int len = snprintf(_btrace_buffer, sizeof(_btrace_buffer),\
            "[%d] 0x%016lx + 0x%.3x: %s\n",       \
            N, symb.addr, symb.offset, symb.name);\
            stdout_function(_btrace_buffer, len);
#else
  #error "Implement me"
#endif

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
                    if (frp(9, ra)) {
                      PRINT_TRACE(9, ra);
                      if (frp(10, ra)) {
                        PRINT_TRACE(10, ra);
                        if (frp(11, ra)) {
                          PRINT_TRACE(11, ra);
                          if (frp(12, ra)) {
                            PRINT_TRACE(12, ra);
                            if (frp(13, ra)) {
                              PRINT_TRACE(13, ra);
                              if (frp(14, ra)) {
                                PRINT_TRACE(14, ra);
  }}}}}}}}}}}}}}}
}
void os::print_backtrace() noexcept
{
  print_backtrace([] (const char* text, size_t length) {
    write(1, text, length);
  });
}

#include <kprint>
extern "C"
void _print_elf_symbols()
{
  const auto& symtab = parser.get_symtab();
  const char* strtab = parser.get_strtab();

  for (size_t i = 0; i < symtab.entries; i++)
  {
    kprintf("%16p: %s\n", (void*) symtab.base[i].st_value, &strtab[symtab.base[i].st_name]);
  }
  kprintf("*** %u entries\n", symtab.entries);
}
void Elf::print_info()
{
  _print_elf_symbols();
}

static struct relocated_header {
  ElfSym*   syms = (ElfSym*) 0x0;
  uint32_t  entries = 0xFFFF;
  uint32_t  strsize = 0xFFFF;
  uint32_t  check_syms = 0xFFFF;
  uint32_t  check_strs = 0xFFFF;

  const char* strings() const {
    return (char*) &syms[entries];
  }
} relocs;

struct elfsyms_header {
  uint32_t  symtab_entries;
  uint32_t  strtab_size;
  uint32_t  sanity_check;
  uint32_t  checksum_syms;
  uint32_t  checksum_strs;
  ElfSym    syms[0];
} __attribute__((packed));

extern "C"
int _get_elf_section_datasize(const void* location)
{
  auto& hdr = *(elfsyms_header*) location;
  // special case for missing call to elf_syms
  if (hdr.symtab_entries == 0) return 0;
  return hdr.symtab_entries * sizeof(ElfSym) + hdr.strtab_size;
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
  const uint32_t temp_hdr = hdr->sanity_check;
  hdr->sanity_check = 0;
  const uint32_t our_sanity = crc32c(hdr, sizeof(elfsyms_header));
  hdr->sanity_check = temp_hdr;
  if (hdr->sanity_check != our_sanity)
  {
    kprintf("ELF syms header CRC failed! "
            "(%08x vs %08x)\n", hdr->sanity_check, our_sanity);
    relocs.entries = 0;
    relocs.strsize = 0;
    return;
  }

  // verify separate checksums of symbols and strings
  uint32_t symbsize = hdr->symtab_entries * sizeof(ElfSym);
  uint32_t csum_syms = crc32c(hdr->syms, symbsize);
  uint32_t csum_strs = crc32c(&hdr->syms[hdr->symtab_entries], hdr->strtab_size);
  if (csum_syms != hdr->checksum_syms || csum_strs != hdr->checksum_strs)
  {
    if (csum_syms != hdr->checksum_syms)
      kprintf("ELF symbol tables checksum failed! "
              "(%08x vs %08x)\n", csum_syms, hdr->checksum_syms);
    if (csum_strs != hdr->checksum_strs)
      kprintf("ELF string tables checksum failed! "
              "(%08x vs %08x)\n", csum_strs, hdr->checksum_strs);
    uint32_t all = crc32c(hdr, sizeof(elfsyms_header) + size);
    kprintf("Checksum ELF section: %08x\n", all);

    relocs.entries = 0;
    relocs.strsize = 0;
    return;
  }
  // update header
  relocs.syms    = (ElfSym*) new_location;
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

extern "C"
void elf_check_symbols_ok()
{
  extern char _ELF_SYM_START_;
  auto* hdr = (elfsyms_header*) &_ELF_SYM_START_;
  // verify CRC sanity check
  const uint32_t temp_hdr = hdr->sanity_check;
  hdr->sanity_check = 0;
  const uint32_t our_sanity = crc32c(hdr, sizeof(elfsyms_header));
  hdr->sanity_check = temp_hdr;
  if (hdr->sanity_check != our_sanity)
  {
    kprintf("ELF syms header CRC failed! "
            "(%08x vs %08x)\n", hdr->sanity_check, our_sanity);
    return;
  }

  // verify separate checksums of symbols and strings
  uint32_t symbsize = hdr->symtab_entries * sizeof(ElfSym);
  uint32_t csum_syms = crc32c(hdr->syms, symbsize);
  uint32_t csum_strs = crc32c(&hdr->syms[hdr->symtab_entries], hdr->strtab_size);
  if (csum_syms != hdr->checksum_syms || csum_strs != hdr->checksum_strs)
  {
    if (csum_syms != hdr->checksum_syms)
      kprintf("ELF symbol tables checksum failed! "
              "(%08x vs %08x)\n", csum_syms, hdr->checksum_syms);
    if (csum_strs != hdr->checksum_strs)
      kprintf("ELF string tables checksum failed! "
              "(%08x vs %08x)\n", csum_strs, hdr->checksum_strs);
    return;
  }
}

#ifdef ARCH_x86_64
#include <kernel/memmap.hpp>
#include <kernel/memory.hpp>
#include <os.hpp>
void elf_protect_symbol_areas()
{
  char* src = (char*) parser.symtab.base;
  ptrdiff_t size = &parser.strtab.base[parser.strtab.size] - src;
  if (size % os::mem::min_psize()) size += os::mem::min_psize() - (size & (os::mem::min_psize()-1));
  if (size == 0) return;
  // create the ELF symbols & strings area
  os::mem::vmmap().assign_range(
      {(uintptr_t) src, (uintptr_t) src + size-1, "Symbols & strings"});

  INFO2("* Protecting syms %p to %p (size %#zx)", src, &src[size], size);
  os::mem::protect((uintptr_t) src, size, os::mem::Access::read);
}
#endif
