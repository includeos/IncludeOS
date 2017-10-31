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

#ifndef FEATURES_H
#define FEATURES_H

/**
 * Enabling / disabling POSIX features
 * Note: libc++ uses <features.h>, newlib <sys/features.h>
 **/

// Newlib needs this switch to enable clock_gettime etc.
#define _POSIX_TIMERS 1
#define _GNU_SOURCE // Provides newest _POSIX_C_SOURCE from newlib
#define _POSIX_MONOTONIC_CLOCK 1

// Enable newlib multibyte support
#define _MB_CAPABLE 1



// Required to pass CMake tests for libc++
#define __GLIBC_PREREQ__(min, maj) 1
#define __GLIBC_PREREQ(min, maj) 1

#include_next <sys/features.h>

#endif
