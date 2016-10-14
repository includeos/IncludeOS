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
typedef _off_t off_t;

void *mmap(void* addr, size_t length,
            int prot,  int flags,
            int fd,    off_t offset);
int munmap(void* addr, size_t length);


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
