#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <vector>
#include "elf_binary.hpp"
#include "../api/util/crc32.hpp"
#include <unistd.h>

static Elf32_Ehdr* elf_header_location;
bool verb = false;

static const Elf32_Ehdr& elf_header() noexcept {
  return *elf_header_location;
}
static const char* elf_offset(int o) noexcept {
  return (char*)elf_header_location + o;
}

static int prune_elf_symbols();
static char* pruned_location = nullptr;
static const char* syms_file = "_elf_symbols.bin";
static const char* syms_section_name = ".elf_symbols";
static const char* SANITY_STRING = "Hello world!";

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

  // Verify that the symbols aren't allready moved
  Elf_binary binary ({fdata, size});
  auto& sh_elf_syms = binary.section_header(syms_section_name);
  auto syms_file_exists = access(syms_file, F_OK ) == 0;
  auto sym_sectionsize_ok = sh_elf_syms.sh_size > 4;
  if (sym_sectionsize_ok and syms_file_exists) {
    fprintf(stderr, "%s: Elf symbols seems to be ok. Nothing to do.\n", args[0]);
    return 0;
  }

  fprintf(stderr, "%s: Pruning ELF symbols \n", args[0]);
  // validate symbols
  elf_header_location = (decltype(elf_header_location)) fdata;
  int pruned_size = prune_elf_symbols();

  // write symbols to binary file
  f = fopen(syms_file, "w");
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

struct relocate_header32 {
  uint32_t  symtab_entries;
  uint32_t  strtab_size;
  uint32_t  sanity_check;
  uint32_t  checksum_syms;
  uint32_t  checksum_strs;
  Elf32_Sym syms[0];
} __attribute__((packed));

static int relocate_pruned_sections(char* new_location, SymTab& symtab, StrTab& strtab)
{
  auto& hdr = *(relocate_header32*) new_location;

  // first prune symbols
  auto*  symloc = hdr.syms;
  size_t symidx = 0;
  for (size_t i = 0; i < symtab.entries; i++)
  {
    auto& cursym = symtab.base[i];
    auto type = ELF32_ST_TYPE(cursym.st_info);
    // we want both functions and untyped, because some 
    // C functions are NOTYPE
    if (type == STT_FUNC || type == STT_NOTYPE) {
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
  // length of symbols & strings
  const size_t size = 
         hdr.symtab_entries * sizeof(Elf32_Sym) +
         hdr.strtab_size * sizeof(char);
  // sanity check
  hdr.sanity_check = crc32(SANITY_STRING, strlen(SANITY_STRING));
  // checksum of symbols & strings and the entire section
  hdr.checksum_syms = crc32(symloc, hdr.symtab_entries * sizeof(Elf32_Sym));
  hdr.checksum_strs = crc32(strloc, hdr.strtab_size);
  uint32_t all = crc32(&hdr, sizeof(relocate_header32) + size);
  fprintf(stderr, "ELF symbols: %08x  "
                  "ELF strings: %08x  "
                  "ELF section: %08x\n",
                  hdr.checksum_syms, hdr.checksum_strs, all);
  // return total length
  return sizeof(relocate_header32) + size;
}

static int prune_elf_symbols()
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
    return relocate_pruned_sections(pruned_location, symtab, strtab);
  }
  // stripped variant
  pruned_location = new char[sizeof(relocate_header32)];
  return sizeof(relocate_header32);
}
