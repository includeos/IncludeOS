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

#include <term>
#include <memdisk>

void Terminal::add_disk_commands(Disk_ptr disk)
{
  // add 'ls' command
  add("ls", "List files in a folder",
    [this, disk] (const std::vector<std::string>& args) -> int
    {
      // current directory, somehow...
      std::string operand = "/";
      if (!args.empty()) operand = args[0];
      
      auto& fs = disk->fs();
      auto vec = fs::new_shared_vector();
      auto err = fs.ls(operand, vec);
      if (!err)
      {
        this->write("%s \t%s \t%s \t%s\r\n", 
            "Name", "Size", "Type", "Sector");
        for (auto& ent : *vec)
        {
          this->write("%s \t%llu \t%s \t%llu\r\n", 
              ent.name().c_str(), ent.size, ent.type_string().c_str(), ent.block);
        }
        this->write("Total %u\r\n", vec->size());
        return 0;
      }
      else
      {
        this->write("Could not list %s\r\n", args[0].c_str());
        return 1;
      }
    });
  // add 'stat' command
  add("stat", "List file information",
    [this, disk] (const std::vector<std::string>& args) -> int
    {
      if (!args.empty())
      {
        auto& fs = disk->fs();
        auto ent = fs.stat(args[0]);
        if (ent.is_valid())
        {
          this->write("%s \t%s \t%s \t%s\r\n", 
              "Name", "Size", "Type", "Sector");
          this->write("%s \t%llu \t%s \t%llu\r\n", 
              ent.name().c_str(), ent.size, ent.type_string().c_str(), ent.block);
          return 0;
        }
        else
        {
          this->write("stat: Cannot stat '%s'\r\n", args[0].c_str());
          return 1;
        }
      }
      else
      {
        this->write("%s\r\n", "stat: Not enough arguments");
        return 1;
      }
    });
  // add 'cat' command
  add("cat", "Concatenate files and print",
    [this, disk] (const std::vector<std::string>& args) -> int
    {
      auto& fs = disk->fs();
      
      for (const auto& file : args)
      {
        // get file information
        auto ent = fs.stat(file);
        if (!ent.is_valid())
        {
          this->write("cat: '%s': No such file or directory\r\n", file.c_str());
          return 1;
        }
        // read file contents
        auto buf = fs.read(ent, 0, ent.size);
        if (!buf.buffer)
        {
          this->write("cat: '%s': I/O error\r\n", file.c_str());
          return 1;
        }
        // write to terminal client
        std::string buffer((char*) buf.buffer.get(), buf.len);
        this->write("%s\r\n", buffer.c_str());
      }
      return 0;
    });
}
