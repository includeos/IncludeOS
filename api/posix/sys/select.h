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
#ifndef POSIX_SYS_SELECT_H
#define POSIX_SYS_SELECT_H

#include <sys/types.h>
#include <sys/signal.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FD_SETSIZE  128


struct fd_set
{
  int fds[FD_SETSIZE];
};
typedef struct fd_set fd_set;


void FD_CLR(int, fd_set *);
int  FD_ISSET(int, fd_set *);
void FD_SET(int, fd_set *);
void FD_ZERO(fd_set *);

int  pselect(int, fd_set *__restrict__, fd_set *__restrict__, fd_set *__restrict__,
         const struct timespec *__restrict__, const sigset_t *__restrict__);
int  select(int, fd_set *__restrict__, fd_set *__restrict__, fd_set *__restrict__,
         struct timeval *__restrict__);

#ifdef __cplusplus
}
#endif
#endif
