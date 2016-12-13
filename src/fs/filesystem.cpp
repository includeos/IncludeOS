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

  void File_system::read_file(const std::string& path, on_read_func on_read) {
    stat(path, [this, on_read, path](error_t err, const Dirent& ent) {
        if(UNLIKELY(err))
          return on_read(err, nullptr, 0);

        if(UNLIKELY(!ent.is_file()))
          return on_read({error_t::E_NOTFILE, path + " is not a file"}, nullptr, 0);

        read(ent, 0, ent.size(), on_read);
      });
  }

  Buffer File_system::read_file(const std::string& path) {
      auto ent = stat(path);

      if(UNLIKELY(!ent.is_valid()))
        return {{error_t::E_NOENT, path + " not found"}, nullptr, 0};

      if(UNLIKELY(!ent.is_file()))
        return {{error_t::E_NOTFILE, path + " is not a file"}, nullptr, 0};

      return read(ent, 0, ent.size());
  }
}
