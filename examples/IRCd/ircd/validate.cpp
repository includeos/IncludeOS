#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <elf.h>

const uint32_t MEMORY_START = 0x200000;
const char* _ELF_DATA;

static const Elf32_Ehdr& elf_header() {
  return *(const Elf32_Ehdr*) _ELF_DATA;
}

struct SymTab {
  Elf32_Sym* base;
  uint32_t   entries;
};
struct StrTab {
  const char* base;
  uint32_t    size;
};
struct relocate_header {
  SymTab symtab;
  StrTab strtab;
};
static relocate_header init_header;
bool _init_elf_parser();

#include <algorithm>

int main(int argc, const char** args)
{
  assert(argc > 1);
  FILE* f = fopen(args[1], "r");
  assert(f);

  // Determine file size
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);

  char* fdata = new char[size];

  rewind(f);
  int res = fread(fdata, sizeof(char), size, f);
  assert(res == size);

  // validate symbols
  _ELF_DATA = fdata;
  assert(_init_elf_parser());
  // show them
  printf("custom symtab at %p (%u entries)\n",
      init_header.symtab.base, init_header.symtab.entries);


  typedef std::pair<Elf32_Sym*, size_t> symbol_size_t;
  std::vector<symbol_size_t> vec;
  int total = 0;
  int func_total = 0;

  for (size_t i = 0; i < init_header.symtab.entries; i++) {
    auto& sym  = init_header.symtab.base[i];
    // only care about functions, and it should be pre-pruned
    //assert (ELF32_ST_TYPE(sym.st_info) == STT_FUNC);
    vec.push_back({&sym, sym.st_size});
    total += sym.st_size;
    if (ELF32_ST_TYPE(sym.st_info) == STT_FUNC) func_total += sym.st_size;
  }
  
  std::sort(vec.begin(), vec.end(),
  [] (const symbol_size_t& s1, const symbol_size_t& s2) -> int {
    return s1.second > s2.second;
  });
  
  
  for (int i = 0; i < 32; i++)
  {
    auto& sym = *vec[i].first;
    const char* name = &init_header.strtab.base[sym.st_name];
    printf("sym [%#x] %s is %d bytes\n", sym.st_value, name, sym.st_size);
  }
  
  printf("Validated! In total %d bytes (functions %d bytes)\n", total, func_total);
  printf("Symtab entries: %u\n", init_header.symtab.entries);
  return 0;
}

bool _init_elf_parser()
{
  SymTab symtab { nullptr, 0 };
  StrTab strtab { nullptr, 0 };
  auto& elf_hdr = elf_header();

  // enumerate all section headers
  printf("ELF header has %u sections\n", elf_hdr.e_shnum);

  auto* shdr = (Elf32_Shdr*) (_ELF_DATA + elf_hdr.e_shoff);

  for (Elf32_Half i = 0; i < elf_hdr.e_shnum; i++)
  {
    switch (shdr[i].sh_type) {
    case SHT_SYMTAB:
      symtab = SymTab { (Elf32_Sym*) (_ELF_DATA + shdr[i].sh_offset),
                        shdr[i].sh_size / sizeof(Elf32_Sym) };
      break;
    case SHT_STRTAB:
      strtab = StrTab { (char*) (_ELF_DATA + shdr[i].sh_offset),
                        shdr[i].sh_size };
      break;
    case SHT_DYNSYM:
    default:
      // don't care tbh
      break;
    }
  }

  printf("full symtab at %p (%u entries)\n", symtab.base, symtab.entries);
  // when not using custom pruned section:
  init_header.symtab = symtab;
  init_header.strtab = strtab;
  return init_header.symtab.entries && init_header.strtab.size;

  for (size_t i = 0; i < symtab.entries; i++)
  {
    auto& sym = symtab.base[i];
    const char* name = &strtab.base[sym.st_name];
    //printf("[%#x]: %s\n", sym.st_value, name);

    if (strcmp("_ELF_SYM_START_", name) == 0) {
      printf("* found elf syms at %#x, trying %#x\n", sym.st_value, sym.st_value - MEMORY_START);
      // calculate offset into file
      const char* header_loc = &_ELF_DATA[sym.st_value - MEMORY_START];
      // reveal header
      const relocate_header& hdr = *(relocate_header*) header_loc;

      init_header.symtab.base = (Elf32_Sym*) (header_loc + sizeof(relocate_header));
      init_header.symtab.entries = hdr.symtab.entries;
      init_header.strtab.base = header_loc + sizeof(relocate_header) +
                                hdr.symtab.entries * sizeof(Elf32_Sym);
      init_header.strtab.size = hdr.strtab.size;
      return init_header.symtab.entries && init_header.strtab.size;
    }
  }
  printf("error: no matching symbol found\n");
  return false;
}
