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

#include <service>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

int main()
{
  int res;

  res = chmod("/dev/null", S_IRUSR);
  printf("chmod result: %d\n", res);
  if (res == -1)
  {
    printf("chmod error: %s\n", strerror(errno));
  }

  res = mkdir("/dev/sda1/root",  S_IWUSR);
  printf("mkdir result: %d\n", res);
  if (res == -1)
  {
    printf("mkdir error: %s\n", strerror(errno));
  }

  res = mkfifo("/dev/null",  S_IWUSR);
  printf("mkfifo result: %d\n", res);
  if (res == -1)
  {
    printf("mkfifo error: %s\n", strerror(errno));
  }

  mode_t old_umask = umask(0);
  printf("Old umask: %d\n", old_umask);

  int fd = STDOUT_FILENO;
  res = fchmod(fd, S_IWUSR);
  printf("fchmod result: %d\n", res);
  if (res == -1)
  {
    printf("fchmod error: %s\n", strerror(errno));
  }

  printf("All done!\n");
  exit(0);
}
