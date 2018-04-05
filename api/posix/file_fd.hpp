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

#ifndef FILE_FD_HPP
#define FILE_FD_HPP

#include "fd.hpp"
#include <fs/dirent.hpp>

class File_FD : public FD {
public:
  explicit File_FD(const int id, const fs::Dirent ent, uint64_t offset = 0)
      : FD(id), ent_ {ent}, offset_ {offset},
        dir_pos{0}, dir_{nullptr}
  {}

  ssize_t read(void*, size_t) override;
  ssize_t readv(const struct iovec*, int iovcnt) override;
  int write(const void*, size_t) override;
  int close() override;
  off_t lseek(off_t, int) override;

  long getdents(struct dirent *dirp, unsigned int count) override;

  bool is_file() override { return true; }

private:
  fs::Dirent ent_;
  uint64_t offset_;
  size_t dir_pos;
  fs::Dirvec_ptr dir_;
};

#endif
