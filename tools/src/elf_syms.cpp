#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <vector>
#include "elf_binary.hpp"
#include "crc32.hpp"
#include <unistd.h>

static char* elf_header_location;
static const char* elf_offset(int o) noexcept {
  return elf_header_location + o;
}

static char* pruned_location = nullptr;
static const char* syms_file = "_elf_symbols.bin";

static int prune_elf_symbols(char*);

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

  auto carch = getenv("ARCH");
  std::string arch = "x86_64";
  if (carch) arch = std::string(carch);
//  fprintf(stderr, "ARCH = %s\n", arch.c_str());

  int pruned_size = 0;
  fprintf(stderr, "%s: Pruning ELF symbols \n", args[0]);

  pruned_size = prune_elf_symbols(fdata);
  // validate size
  assert(pruned_size != 0);

  // write symbols to binary file
  f = fopen(syms_file, "w");
  assert(f);
  fwrite(pruned_location, sizeof(char), pruned_size, f);
  fclose(f);
  return 0;
}

struct SymTab {
  const char* base;
  uint32_t    entries;
};
struct StrTab {
  const char* base;
  uint32_t    size;

  StrTab(const char* base, uint32_t size) : base(base), size(size) {}
};

struct relocate_header
{
  uint32_t  symtab_entries;
  uint32_t  strtab_size;
  uint32_t  sanity_check;
  uint32_t  checksum_syms;
  uint32_t  checksum_strs;
  char      data[0];
};

template <typename ElfSym, int Bits>
static int relocate_pruned(char* new_location, SymTab& symtab, StrTab& strtab)
{
  auto& hdr = *(relocate_header*) new_location;

  // first prune symbols
  auto*  symloc = (ElfSym*) hdr.data;
  size_t symidx = 0;
  for (size_t i = 0; i < symtab.entries; i++)
  {
    auto& cursym = ((const ElfSym*) symtab.base)[i];
    int   type;
    if (Bits == 32)
        type = ELF32_ST_TYPE(cursym.st_info);
    else if (Bits == 64)
        type = ELF64_ST_TYPE(cursym.st_info);
    else
        throw std::runtime_error("Invalid bits");
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
         hdr.symtab_entries * sizeof(ElfSym) +
         hdr.strtab_size * sizeof(char);

  // checksum of symbols & strings and the entire section
  hdr.checksum_syms = crc32c(symloc, hdr.symtab_entries * sizeof(ElfSym));
  hdr.checksum_strs = crc32c(strloc, hdr.strtab_size);
  uint32_t hdr_csum = crc32c(&hdr, sizeof(relocate_header) + size);
  fprintf(stderr, "ELF symbols: %08x  "
                  "ELF strings: %08x  "
                  "ELF section: %08x\n",
                  hdr.checksum_syms, hdr.checksum_strs, hdr_csum);
  hdr.sanity_check = 0;
  // header consistency check
  hdr.sanity_check = crc32c(new_location, sizeof(relocate_header));

  // return total length
  return sizeof(relocate_header) + size;
}

template <int Bits, typename ElfEhdr, typename ElfShdr, typename ElfSym>
static int prune_elf_symbols()
{
  SymTab symtab { nullptr, 0 };
  std::vector<StrTab> strtabs;
  auto& elf_hdr = *(ElfEhdr*) elf_header_location;
  //printf("ELF header has %u sections\n", elf_hdr.e_shnum);

  auto* shdr = (ElfShdr*) elf_offset(elf_hdr.e_shoff);

  for (auto i = 0; i < elf_hdr.e_shnum; i++)
  {
    switch (shdr[i].sh_type) {
    case SHT_SYMTAB:
      symtab = SymTab { elf_offset(shdr[i].sh_offset),
                        (uint32_t) shdr[i].sh_size / (uint32_t) sizeof(ElfSym) };
      break;
    case SHT_STRTAB:
      strtabs.emplace_back(elf_offset(shdr[i].sh_offset), shdr[i].sh_size);
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
        new char[sizeof(relocate_header) + symtab.entries * sizeof(ElfSym) + strtab.size];
    return relocate_pruned<ElfSym, Bits>(pruned_location, symtab, strtab);
  }
  // stripped variant
  pruned_location = new char[sizeof(relocate_header)];
  memset(pruned_location, 0, sizeof(relocate_header));
  return sizeof(relocate_header);
}

static int prune_elf32_symbols(char* location)
{
  elf_header_location = location;
  return prune_elf_symbols<32, Elf32_Ehdr, Elf32_Shdr, Elf32_Sym> ();
}
static int prune_elf64_symbols(char* location)
{
  elf_header_location = location;
  return prune_elf_symbols<64, Elf64_Ehdr, Elf64_Shdr, Elf64_Sym> ();
}

static int prune_elf_symbols(char* location)
{
  auto* hdr = (Elf32_Ehdr*) location;
  assert(hdr->e_ident[EI_MAG0] == ELFMAG0);
  assert(hdr->e_ident[EI_MAG1] == ELFMAG1);
  assert(hdr->e_ident[EI_MAG2] == ELFMAG2);
  assert(hdr->e_ident[EI_MAG3] == ELFMAG3);

  if (hdr->e_ident[EI_CLASS] == ELFCLASS32)
      return prune_elf32_symbols(location);
  else if (hdr->e_ident[EI_CLASS] == ELFCLASS64)
      return prune_elf64_symbols(location);
  assert(0 && "Unknown ELF class");
  return 0;
}
