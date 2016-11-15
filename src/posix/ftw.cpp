// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#include <string>
#include <ftw.h>
#include <memdisk>

extern fs::Disk_ptr& fs_disk();

class Walker {
public:
  explicit Walker(Nftw_func fn, int flags) : fn_ptr_ {fn}, flags_ {flags} {};

  int walk(const std::string& path) noexcept {
    struct stat buffer {};
    struct FTW ftw {};

    auto ent = fs_disk()->fs().stat(path);

    int res = stat(path.c_str(), &buffer);
    if (res != -1) {
      if (S_ISDIR(buffer.st_mode) == 0) {
        // path is a file
        return fn_ptr_(path.c_str(), &buffer, FTW_F, &ftw);
      }
      else {
        // path is a directory
        int result;
        if (not (flags_ & FTW_DEPTH)) {
          // call fn on dir first
          result = fn_ptr_(path.c_str(), &buffer, FTW_D, &ftw);
        }
        // call fn on each entry
        fs_disk()->fs().ls(ent, [path, &result, this](auto, auto entries) {
          for (auto&& ent : *entries) {
            if (ent.name() == "." or ent.name() == "..")
            {
              //printf("Skipping %s\n", ent.name().c_str());
            }
            else
            {
              if ((result = walk(path + "/" + ent.name()) != 0)) {
                break;
              }
            }
          }
        });
        if (flags_ & FTW_DEPTH) {
          result = fn_ptr_(path.c_str(), &buffer, FTW_D, &ftw);
        }
        return result;
      }
  }
  else {
    // could not stat path
    int result;
    if (path == "/") {
      auto ents = fs_disk()->fs().ls("/").entries;
      for (auto&& ent : *ents) {
        if (ent.is_valid()) {
          if ((result = walk("/" + ent.name()) != 0)) {
            break;
          }
        }
      }
      return result;
    }
    else {
      return fn_ptr_(path.c_str(), &buffer, FTW_NS, &ftw);
    }
  }
}
private:
  Nftw_func* fn_ptr_;
  int flags_;
  int level_ {0};
};

int nftw(const char* path, Nftw_func fn, int fd_limit, int flags) {
  Walker walker {fn, flags};
  return walker.walk(path);
}
