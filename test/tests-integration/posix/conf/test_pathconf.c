// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>

#define PCV(x) print_conf_value(x, #x)

const char* filename = "/etc/passwd";

static void print_conf_value(int name, const char* sym) {
  errno = 0;
  int fd = open(filename, O_RDONLY);
  assert(fd > 0);
  if (fd == -1) {
    printf("Couldn't open %s\n", filename);
    exit(1);
  }
  long value = fpathconf(fd, name);
  if (value == -1) {
    errno == 0 ? printf("%s (%d): No limit\n", sym, name)
      : printf("%s pathconf error: %s\n", sym, strerror(errno));
  }
  else {
    printf("%s (%d): %ld\n", sym, name, value);
  }
}

void test_pathconf() {
  PCV(_PC_FILESIZEBITS);
  PCV(_PC_LINK_MAX);
  PCV(_PC_MAX_CANON);
  PCV(_PC_MAX_INPUT);
  PCV(_PC_NAME_MAX);
  PCV(_PC_PATH_MAX);
  PCV(_PC_PIPE_BUF);
  PCV(_PC_2_SYMLINKS);
  PCV(_PC_ALLOC_SIZE_MIN);
  PCV(_PC_REC_INCR_XFER_SIZE);
  PCV(_PC_REC_MAX_XFER_SIZE);
  PCV(_PC_REC_MIN_XFER_SIZE);
  PCV(_PC_REC_XFER_ALIGN);
  PCV(_PC_SYMLINK_MAX);
  PCV(_PC_CHOWN_RESTRICTED);
  PCV(_PC_NO_TRUNC);
  PCV(_PC_VDISABLE);
  PCV(_PC_ASYNC_IO);
  PCV(_PC_PRIO_IO);
  PCV(_PC_SYNC_IO);
#ifdef _PC_TIMESTAMP_RESOLUTION
  PCV(_PC_TIMESTAMP_RESOLUTION);
#endif
}
