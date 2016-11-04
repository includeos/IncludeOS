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
#ifndef POSIX_UCONTEXT_H
#define POSIX_UCONTEXT_H

#include <os>

struct mcontext_t {
  size_t edi;
  size_t esi;
  size_t ebp;
  size_t ebx;
  size_t edx;
  size_t ecx;
  size_t eax;

  uint8_t floating_point_env[28];
  size_t eip;
  size_t flags;
  size_t ret_esp;
};

struct ucontext_t {
  ucontext_t* uc_link;
  stack_t uc_stack;
  mcontext_t uc_mcontext;

public:
    static constexpr size_t MAX_STACK_SIZE = 4096 * 3; // Bytes
};

#ifdef __cplusplus
extern "C" {
#endif

int getcontext(ucontext_t *ucp);
int setcontext(const ucontext_t *ucp);
void makecontext(ucontext_t *ucp, void (*func)(), int argc, ...);
int swapcontext(ucontext_t *oucp, ucontext_t *ucp);

#ifdef __cplusplus
}
#endif

#endif
