// Part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud
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
#include <stdint.h>
#include <string>

// Index node
typedef int16_t inode_t;

#define likely(x)   __builtin_expect(!!(x), true)
#define unlikely(x) __builtin_expect(!!(x), false) 

#include <sys/errno.h>

/*
// No such file or directory
#define ENOENT   2
// I/O Error
#define EIO      5
// File already exists
#define EEXIST  17
// Not a directory
#define ENOTDIR 20
// Invalid argument
#define EINVAL  22
// No space left on device
#define ENOSPC  28
// Directory not empty
#define ENOTEMPTY 39
*/

extern std::string fs_error_string(int error);
