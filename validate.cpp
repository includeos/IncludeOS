#include <cstdio>
#include <cstdint>
#include <cassert>
#include <elf.h>

static const char* filename = "IRCd.img";
const char* _ELF_DATA;

static const Elf32_Ehdr& elf_header() {
  return *(const Elf32_Ehdr*) _ELF_DATA;
}

bool _init_elf_parser();

int main()
{
  FILE* f = fopen(filename, "r");
  assert(f);

  // Determine file size
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);

  char* fdata = new char[size];
  
  rewind(f);
  int res = fread(fdata, sizeof(char), size, f);
  assert(res == size);
  
  // validate symbols
  _ELF_DATA = fdata + 512;
  assert(_init_elf_parser());
  
  return 0;
}

struct SymTab {
  Elf32_Sym* base;
  uint32_t   entries;
};
struct StrTab {
  char*    base;
  uint32_t size;
};
struct relocate_header {
  SymTab symtab;
  StrTab strtab;
};
static relocate_header init_header;

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
  
  printf("symtab at %p (%u entries)\n", symtab.base, symtab.entries);
  
  for (size_t i = 0; i < symtab.entries; i++)
  {
    auto& sym = symtab.base[i];
    printf("[%#x]: %s\n", sym.st_value, &strtab.base[sym.st_name]);
  }
  
  init_header.symtab = symtab;
  init_header.strtab = strtab;
  
  // nothing to do if stripped
  return symtab.entries && strtab.size;
}
