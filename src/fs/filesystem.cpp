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

#include <common>
#include <array>

#include <fs/dirent.hpp>

namespace fs
{
  error_t no_error { error_t::NO_ERR, "" };

  const std::string& error_t::token() const noexcept {
    const static std::array<std::string, 6> tok_str
    {{
      "No error",
      "General I/O error",
      "Mounting filesystem failed",
      "No such entry",
      "Not a directory",
      "Not a file"
    }};

    return tok_str[token_];
  }

  void File_system::read_file(const std::string& path, on_read_func on_read) const
  {
    stat(
      path,
      on_stat_func::make_packed(
      [this, on_read, path](error_t err, const Dirent& ent)
      {
        if(UNLIKELY(err))
          return on_read(err, nullptr);

        if(UNLIKELY(!ent.is_file()))
          return on_read({error_t::E_NOTFILE, path + " is not a file"}, nullptr);

        read(ent, 0, ent.size(), on_read);
      })
    );
  }

  Buffer File_system::read_file(const std::string& path) const {
      auto ent = stat(path);

      if(UNLIKELY(!ent.is_valid()))
        return {{error_t::E_NOENT, path + " not found"}, nullptr};

      if(UNLIKELY(!ent.is_file()))
        return {{error_t::E_NOTFILE, path + " is not a file"}, nullptr};

      return read(ent, 0, ent.size());
  }

  static error_t print_subtree(Dirvec_ptr entries, int depth)
  {
    int indent = depth * 3;
    for (auto entry : *entries)
    {
      if (entry.is_dir()) {
        // Directory
        if (entry.name() != "."  and entry.name() != "..") {
          printf(" %*s-[ %s ]\n", indent, "+", entry.name().c_str());
          // list entries in directory
          auto list = entry.ls();
          if (list.error) return list.error;
          // print subtree from that directory
          auto err = print_subtree(list.entries, depth + 1);
          if (err) return err;
        } else {
          printf(" %*s  %s\n", indent, "+", entry.name().c_str());
        }
      } else {
        // File (.. or other things)
        printf(" %*s-> %s\n", indent, "+", entry.name().c_str());
      }
    }
    printf(" %*s \n", indent, " ");
    return no_error;
  }
  error_t File_system::print_subtree(const std::string& path)
  {
    auto list = ls(path);
    if (list.error) return list.error;
    return fs::print_subtree(list.entries, 0);
  }
}
