#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <vector>
#include "elf.h"

static Elf32_Ehdr* elf_header_location;

static const Elf32_Ehdr& elf_header() noexcept {
  return *elf_header_location;
}
static const char* elf_offset(int o) noexcept {
  return (char*)elf_header_location + o;
}

static void prune_elf_symbols();
static char* pruned_location;
static int   pruned_size = 0;

int main(int argc, const char** args)
{
  assert(argc > 1);
  FILE* f = fopen(args[1], "r");
  assert(f);

  // Determine file size
  fseek(f, 0, SEEK_END);
  int size = ftell(f);

  char* fdata = new char[size];

  rewind(f);
  int res = fread(fdata, sizeof(char), size, f);
  assert(res == size);
  fclose(f);

  // validate symbols
  elf_header_location = (decltype(elf_header_location)) fdata;
  prune_elf_symbols();

  // write symbols to binary file
  f = fopen("_elf_symbols.bin", "w");
  assert(f);
  fwrite(pruned_location, sizeof(char), pruned_size, f);
  fclose(f);
  return 0;
}

struct SymTab {
  Elf32_Sym* base;
  uint32_t   entries;
};
struct StrTab {
  char*    base;
  uint32_t size;

  StrTab(char* base, uint32_t size) : base(base), size(size) {}
};
struct relocate_header {
  SymTab symtab;
  StrTab strtab;
};

struct relocate_header32 {
  uint32_t symtab_base;
  uint32_t symtab_entries;
  uint32_t strtab_base;
  uint32_t strtab_size;
};

static int relocate_pruned_sections(char* new_location, SymTab& symtab, StrTab& strtab)
{
  auto& hdr = *(relocate_header32*) new_location;

  // first prune symbols
  auto*  symloc = (Elf32_Sym*) (new_location + sizeof(relocate_header32));
  size_t symidx = 0;
  for (size_t i = 0; i < symtab.entries; i++)
  {
    auto& cursym = symtab.base[i];
    if (ELF32_ST_TYPE(cursym.st_info) == STT_FUNC) {
      symloc[symidx++] = cursym;
    }
  }
  // new total symbol entries
  hdr.symtab_entries = symidx;

  // move strings (one by one)
  char*  strloc = (char*) &symloc[hdr.symtab_entries];
  size_t index  = 0;
  for (size_t i = 0; i < hdr.symtab_entries; i++)
  {
    auto& sym = symloc[i];
    // get original location and length
    const char* org = &strtab.base[sym.st_name];
    size_t      len = strlen(org) + 1;
    // set new symbol name location
    sym.st_name = index; // = distance from start
    // insert string into new location
    memcpy(&strloc[index], org, len);
    index += len;
  }
  // new entry base and total length
  hdr.strtab_size = index;
  // length of the whole thing
  return sizeof(relocate_header32) +
         hdr.symtab_entries * sizeof(Elf32_Sym) +
         hdr.strtab_size * sizeof(char);
}

static void prune_elf_symbols()
{
  SymTab symtab { nullptr, 0 };
  std::vector<StrTab> strtabs;
  auto& elf_hdr = elf_header();
  //printf("ELF header has %u sections\n", elf_hdr.e_shnum);

  auto* shdr = (Elf32_Shdr*) elf_offset(elf_hdr.e_shoff);

  for (Elf32_Half i = 0; i < elf_hdr.e_shnum; i++)
  {
    switch (shdr[i].sh_type) {
    case SHT_SYMTAB:
      symtab = SymTab { (Elf32_Sym*) elf_offset(shdr[i].sh_offset),
                        shdr[i].sh_size / (int) sizeof(Elf32_Sym) };
      break;
    case SHT_STRTAB:
      strtabs.emplace_back((char*) elf_offset(shdr[i].sh_offset), shdr[i].sh_size);
      break;
    case SHT_DYNSYM:
    default:
      break; // don't care tbh
    }
  }
  //printf("symtab at %p (%u entries)\n", symtab.base, symtab.entries);

  // not stripped
  if (symtab.entries && !strtabs.empty()) {
    StrTab strtab = *std::max_element(std::begin(strtabs), std::end(strtabs),
                    [](auto const& lhs, auto const& rhs) { return lhs.size < rhs.size; });

    // allocate worst case, guaranteeing we have enough space
    pruned_location =
        new char[sizeof(relocate_header32) + symtab.entries * sizeof(Elf32_Sym) + strtab.size];
    pruned_size = relocate_pruned_sections(pruned_location, symtab, strtab);
    return;
  }
  // stripped variant
  pruned_location = new char[sizeof(relocate_header32)];
  pruned_size = sizeof(relocate_header32);
}
