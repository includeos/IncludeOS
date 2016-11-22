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
#include <unistd.h>
#include <fd_map.hpp>
#include <kernel/os.hpp>
#include <kernel/rng.hpp>

static const int syscall_fd   {999};
static const int rng_fd       {998};
static bool debug_syscalls    {true};

int open(const char* s, int, ...)
{
  if(strcmp(s, "/dev/random") == 0 || strcmp(s, "/dev/urandom") == 0) {
    return rng_fd;
  }
  return -1;
}

int close(int fd)
{
  if(fd == rng_fd) {
    return 0;
  }
  try
  {
    return FD_map::_close(fd);
  }
  catch(const FD_not_found&)
  {
    errno = EBADF;
  }
  return -1;
}

int read(int fd, void* buf, size_t len)
{
  if(fd == rng_fd) {
    rng_extract(buf, len);
    return len;
  }
  return 0;
}
int write(int fd, const void* ptr, size_t len)
{
  if (fd < 4) {
    return OS::print((const char*) ptr, len);
  }
  else if (fd == rng_fd) {
    rng_absorb(ptr, len);
    return len;
  }
  try {
    auto& fildes = FD_map::_get(fd);
    return fildes.write(ptr, len);
  }
  catch(const FD_not_found&) {
    errno = EBADF;
    return -1;
  }
}

// read value of a symbolic link (which we don't have)
ssize_t readlink(const char* path, char*, size_t bufsiz)
{
  printf("readlink(%s, bufsize=%u)\n", path, bufsiz);
  return 0;
}

int fsync(int fildes)
{
  try {
    (void) fildes;
    //auto& fd = FD_map::_get(fildes);
    // files should return 0, and others should not
    return 0;
  }
  catch(const FD_not_found&) {
    errno = EBADF;
    return -1;
  }
}

int fchown(int, uid_t, gid_t)
{
  return -1;
}

#include <kernel/irq_manager.hpp>
#include <kernel/rtc.hpp>
unsigned int sleep(unsigned int seconds)
{
  int64_t now  = RTC::now();
  int64_t done = now + seconds;
  while (true) {
    if (now >= done) break;
    OS::block();
    now = RTC::now();
  }
  return 0;
}
