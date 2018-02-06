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

#include <file_fd.hpp>
#include <errno.h>
#include <posix_strace.hpp>

int File_FD::read(void* p, size_t n) {
  auto buf = ent_.read(offset_, n);
  memcpy(p, buf.data(), std::min(n, buf.size()));
  offset_ += buf.size();
  return buf.size();
}

int File_FD::write(const void*, size_t) {
  return -1;
}

int File_FD::close() {
  return 0;
}

int File_FD::lseek(off_t offset, int whence)
{
  if ((whence != SEEK_SET) && (whence != SEEK_CUR) && (whence != SEEK_END)) {
    PRINT("lseek(%lu, %d) == %d\n", offset, whence, -1);
    errno = EINVAL;
    return -1;
  }
  off_t calculated_offset = offset_;
  if (whence == SEEK_SET) {
    calculated_offset = offset;
  }
  else if (whence == SEEK_CUR) {
    calculated_offset += offset;
  }
  else if (whence == SEEK_END) {
    const off_t end = ent_.size();
    calculated_offset = end + offset;
  }
  if (calculated_offset < 0) calculated_offset = 0;
  offset_ = calculated_offset;
  PRINT("lseek(%lu, %d) == %d\n", offset, whence, offset_);
  return offset_;
}
