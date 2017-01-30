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

#ifndef SYS_FEATURES_H
#define SYS_FEATURES_H

// Newlib needs this switch to enable clock_gettime etc.
#define _POSIX_TIMERS 1

// Required to pass CMake tests for libc++
#define __GLIBC_PREREQ__(min, maj) 1
#define __GLIBC_PREREQ(min, maj) 1

#include_next <sys/features.h>

#endif
