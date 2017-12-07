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

#include <fcntl.h>
#include <fd_map.hpp>
#include <cstdarg>
#include <errno.h>
#include <posix_strace.hpp>

int  creat(const char* filename, mode_t mode)
{
  PRINT("creat(%s, %x) = -1\n", filename, mode);
  errno = EINVAL;
  return -1;
}
int fcntl(int fd, int cmd, ... /* arg */ )
{
  PRINT("fcntl(%d, %d, ...)\n", fd, cmd);
  try {
    auto& desc = FD_map::_get(fd);
    va_list va;
    va_start(va, cmd);
    int ret = desc.fcntl(cmd, va);
    va_end(va);
    return ret;
  }
  catch(const FD_not_found&) {
    errno = EBADF;
    return -1;
  }
}

int  posix_fadvise(int fd, off_t, off_t, int)
{
  PRINT("posix_fadvise(%d) = -1\n", fd);
  return -1;
}
int  posix_fallocate(int fd, off_t, off_t)
{
  PRINT("opendir(%d) = -1\n", fd);
  return -1;
}
