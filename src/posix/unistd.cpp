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
#include <memdisk>

int open(const char*, int, ...)
{
  return -1;
}

int close(int fildes)
{
  try
  {
    return FD_map::_close(fildes);
  }
  catch(const FD_not_found&)
  {
    errno = EBADF;
  }
  return -1;
}

int read(int, void*, size_t)
{
  return 0;
}
int write(int file, const void* ptr, size_t len)
{
  if (file < 4) {
    return OS::print((const char*) ptr, len);
  }
  try {
    auto& fd = FD_map::_get(file);
    return fd.write(ptr, len);
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

static std::string cwd;

fs::Disk_ptr& fs_disk() {
  static fs::Disk_ptr disk = fs::new_shared_memdisk();
  static bool mounted = false;
  if (not mounted)
  {
    disk->mount([](fs::error_t err) {
      if (err) {
        printf("ERROR MOUNTING DISK\n");
      exit(127);
      }
    });
  }
  mounted = true;
  cwd = "/";
  return disk;
}

int chdir(const char *path)
{
  if (not path or strlen(path) < 2)
  {
    errno = ENOENT;
    return -1;
  }
  auto ent = fs_disk()->fs().stat(path);
  if (ent.is_dir())
  {
    // path is a dir
    cwd.assign(path);
    return 0;
  }
  else
  {
    // path is not a dir
    errno = ENOTDIR;
    return -1;
  }
}

char *getcwd(char *buf, size_t size)
{
  if (size == 0)
  {
    errno = EINVAL;
    return nullptr;
  }
  if ((cwd.length() + 1) < size)
  {
    snprintf(buf, size, "%s", cwd.c_str());
    return buf;
  }
  else
  {
    errno = ERANGE;
    return nullptr;
  }
}
