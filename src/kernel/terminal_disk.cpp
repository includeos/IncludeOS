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
#include <fs/disk.hpp>
#include <fs/path.hpp>

using namespace fs;

int target_directory(Disk_ptr disk, const Path& path)
{
  // avoid stat on root directory
  if (path.empty()) return 0;

  std::string strpath(path.to_string());

  auto& fs = disk->fs();
  auto ent = fs.stat(strpath);

  if (!ent.is_valid())
    return 1;
  else if (!ent.is_dir())
    return 1;
  else
    return 0;
}

void Terminal::add_disk_commands(Disk_ptr disk)
{
  auto curdir = std::make_shared<std::string> ("/");

  // add 'cd' command
  add("cd", "Change current directory",
  [this, curdir, disk] (const std::vector<std::string>& args) -> int
  {
    // current directory, somehow...
    std::string target = "/";
    if (!args.empty()) target = args[0];

    Path path(*curdir);
    path += target;

    int rv = target_directory(disk, path);
    if (rv)
    {
      this->write("cd: %s: No such file or directory\r\n", target.c_str());
      return rv;
    }
    *curdir = path.to_string();
    return 0;
  });
  // add 'ls' command
  add("ls", "List files in a folder",
  [this, curdir, disk] (const std::vector<std::string>& args) -> int
  {
    // current directory, somehow...
    Path path(*curdir);
    if (!args.empty()) path += args[0];

    int rv = target_directory(disk, path);
    if (rv)
    {
      this->write("ls: %s: No such file or directory\r\n", path.to_string().c_str());
      return rv;
    }

    std::string target = path.to_string();

    auto& fs = disk->fs();
    auto list = fs.ls(target);
    if (!list.error)
    {
      this->write("%s \t%s \t%s \t%s\r\n",
                  "Name", "Size", "Type", "Sector");
      for (auto& ent : *list.entries)
      {
        this->write("%s \t%llu \t%s \t%llu\r\n",
          ent.name().c_str(), ent.size(), ent.type_string().c_str(), ent.block);
      }
      this->write("Total %u\r\n", list.entries->size());
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
                    ent.name().c_str(), ent.size(), ent.type_string().c_str(), ent.block);
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
      auto buf = fs.read(ent, 0, ent.size());
      if (not buf.is_valid())
      {
        this->write("cat: '%s': I/O error\r\n", file.c_str());
        return 1;
      }
      // write to terminal client
      std::string buffer((char*) buf.data(), buf.size());
      this->write("%s\r\n", buffer.c_str());
    }
    return 0;
  });
}
