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

#include <service>
#include <cstdio>
#include <net/inet4>


#include <cassert>
#include "elf.h"

struct SymTab {
  Elf32_Sym* base;
  uint32_t   entries;
};
struct StrTab {
  char*    base;
  uint32_t size;
};

void begin_update(const char* blob, size_t size)
{
  // move binary to 32mb
  void* update_area = (void*) 0x2000000;
  memcpy(update_area, blob, size);
  
  // discover entry point
  const char* binary = (char*)update_area + 512;
  Elf32_Ehdr& hdr = *(Elf32_Ehdr*) binary;
  
  assert(hdr.e_ident[1] == 'E');
  assert(hdr.e_ident[2] == 'L');
  assert(hdr.e_ident[3] == 'F');
  
  printf("ELF ident: %*s\n", 4, hdr.e_ident);
  
  // the function we want to jump to:
  const char* DESTINATION = "_start";
  
  // find symbols and strings
  SymTab symtab { nullptr, 0 };
  StrTab strtab { nullptr, 0 };
  
  // ... in section headers
  auto* shdr = (Elf32_Shdr*) (binary + hdr.e_shoff);
  for (Elf32_Half i = 0; i < hdr.e_shnum; i++)
  {
    switch (shdr[i].sh_type) {
    case SHT_SYMTAB:
      symtab = { (Elf32_Sym*) (binary + shdr[i].sh_offset),
                 shdr[i].sh_size / sizeof(Elf32_Sym) };
      break;
    case SHT_STRTAB:
      strtab = { (char*) (binary + shdr[i].sh_offset),
                 shdr[i].sh_size };
      break;
    case SHT_DYNSYM:
    default:
      // don't care tbh
      break;
    }
  }
  // nothing to do if stripped
  if (symtab.entries == 0 || strtab.size == 0)
  {
    // what now?
    printf("* Cannot live update with no symbols\n");
    return;
  }
  
  printf("* Found %u symbols, ", symtab.entries);
  printf("string table is %u b\n", strtab.size);
  void (*entry_func) () = nullptr;

  // search for function we are looking for
  for (Elf32_Sym* sym = symtab.base; 
        sym < symtab.base + symtab.entries; sym++)
  {
    // functions only
    //if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC)
    // match in strtab against the function we are looking for
    if (strcmp(DESTINATION, &strtab.base[sym->st_name]) == 0)
    {
      const char* func = &strtab.base[sym->st_name];
      printf("Found function %s located at %#x\n", func, sym->st_value);
      entry_func = (void(*)()) sym->st_value;
    }
  }

  if (entry_func == nullptr)
  {
    printf("* Could not find entry function %s\n", DESTINATION);
    return;
  }
  
  // replace ourselves and reset by jumping to _start
  printf("* Jumping to new service...\n\n");
  memcpy((void*) 0x200000, binary, size);
  entry_func();
}

void Service::start(const std::string&)
{
  static char*  update_blob = new char[1024*1024*10];
  static size_t update_size = 0;

  auto& inet = net::Inet4::ifconfig<0>(
        { 10,0,0,42 },     // IP
        { 255,255,255,0 }, // Netmask
        { 10,0,0,1 },      // Gateway
        { 10,0,0,1 });     // DNS
  
  auto& server = inet.tcp().bind(666);
  server.on_connect(
  [] (auto conn)
  {
    // reset update chunk
    update_size = 0;
    // retrieve binary
    conn->on_read(9000,
    [conn] (net::tcp::buffer_t buf, size_t n)
    {
      memcpy(update_blob + update_size, buf.get(), n);
      update_size += n;
      
    }).on_close(
    [] {
      printf("New update size: %u b\n", update_size);
      begin_update(update_blob, update_size);
    });
  });
}
