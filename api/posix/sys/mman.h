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

#pragma once
#ifndef POSIX_SYS_MMAN_H
#define POSIX_SYS_MMAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <sys/types.h>
typedef _off_t off_t;

struct posix_typed_mem_info
{
  size_t  posix_tmi_length;
};

int    mlock(const void *, size_t);
int    mlockall(int);
void  *mmap(void *, size_t, int, int, int, off_t);
int    mprotect(void *, size_t, int);
int    msync(void *, size_t, int);
int    munlock(const void *, size_t);
int    munlockall(void);
int    munmap(void *, size_t);
int    posix_madvise(void *, size_t, int);
int    posix_mem_offset(const void *__restrict__, size_t, off_t *__restrict__,
           size_t *__restrict__, int *__restrict__);
int    posix_typed_mem_get_info(int, struct posix_typed_mem_info *);
int    posix_typed_mem_open(const char *, int, int);
int    shm_open(const char *, int, mode_t);
int    shm_unlink(const char *);

// Page can be executed.
#define PROT_EXEC     0x1
// Page cannot be accessed.
#define PROT_NONE     0x2
// Page can be read.
#define PROT_READ     0x4
// Page can be written.
#define PROT_WRITE    0x8


// Interpret addr exactly.
#define MAP_FIXED     0x1
// Changes are private.
#define MAP_PRIVATE   0x2
// Share changes.
#define MAP_SHARED    0x4

// Returned on failure to allocate pages
#define MAP_FAILED    ((void*)-1)

#ifdef __cplusplus
}
#endif

#endif
