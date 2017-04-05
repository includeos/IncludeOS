// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#include <os>
#include <util/elf_binary.hpp>
#include <util/sha1.hpp>

bool verb = true;

#define MYINFO(X,...) INFO("Service", X, ##__VA_ARGS__)

extern "C" void hotswap(const char* base, int len, char* dest, void* start);

void Service::start(const std::string& args)
{
  MYINFO("Testing kernel modules. Args: %s", args.c_str());

  auto mods = OS::modules();

  //Expects(mods.size() == 3);

  printf("Found %i modules: \n", mods.size());

  for (auto mod : mods)
    INFO2("* %s @ 0x%x - 0x%x, size: %ib",
           reinterpret_cast<char*>(mod.cmdline),
          mod.mod_start, mod.mod_end, mod.mod_end - mod.mod_start);

  // Verify module cmdlines
  Expects(std::string((char*)mods[0].cmdline) == "../mod1.json");
  Expects(std::string((char*)mods[1].cmdline) == "../seed loaded as module");
  Expects(std::string((char*)mods[2].cmdline) == "../mod3.json");

  // verify content of text modules
  Expects(std::string((char*)mods[0].mod_start)
          == "{\"module1\" : \"JSON data\" }\n");

  Expects(std::string((char*)mods[2].mod_start)
          == "{\"module3\" : \"More JSON data, for mod2 service\" }\n");

  multiboot_module_t binary = mods[1];

  MYINFO("Verifying mod2 as ELF binary");
  Elf_binary elf ({(char*)binary.mod_start, (int)(binary.mod_end - binary.mod_start)});

  MYINFO("Moving hotswap function (now at %p)", &hotswap);
  memcpy((void*)0x8000, (void*)&hotswap, 1024);

  MYINFO("Preparing for jump to %s", (char*)binary.cmdline);

  char* base  = (char*)binary.mod_start;
  int len = (int)(binary.mod_end - binary.mod_start);
  char* dest = (char*)0x100000;
  void* start = (void*)elf.entry();

  SHA1 sha;
  sha.update(base, len);
  MYINFO("Sha1 of ELF binary module: %s", sha.as_hex().c_str());


  MYINFO("Jump params: base: %p, len: %i, dest: %p, start: %p",
         base, len, dest, start);

  MYINFO("Disabling interrupts and calling hotswap...");

  asm("cli");
  ((decltype(&hotswap))0x8000)(base, len, dest, start);

  panic("Should have jumped\n");

}
