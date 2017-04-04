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

void Service::start(const std::string& args)
{
  printf("Testing kernel modules. Args: %s \n", args.c_str());

  auto mods = OS::modules();

  Expects(mods.size() == 3);

  printf("Found %i modules: \n", mods.size());

  for (auto mod : mods)
    printf("\t* %s @ 0x%x - 0x%x, size: %ib \n",
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

  // TODO: Properly verify mod2 as ELF binary
  Expects(((uint8_t*) mods[1].mod_start)[0] == 0x7f);
  Expects(((char*) mods[1].mod_start)[1] == 'E');
  Expects(((char*) mods[1].mod_start)[2] == 'L');
  Expects(((char*) mods[1].mod_start)[3] == 'F');

  exit(0);
}
