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

#pragma once
#include <cstdint>
#include <cstddef>

#define SYS_WRITE  1


#define SYSCALL_1(num, arg1)            \
  asm ("movq %0, %%rdi" : : "r"(arg1)); \
  asm ("movq %0, %%rax" : : "i"(num));  \
  asm ("syscall; movl %0, %%eax" : "=r"(result));

#define SYSCALL_2(num, arg1, arg2)      \
  asm ("movq %0, %%rdi" : : "r"(arg1)); \
  asm ("movq %0, %%rsi" : : "r"(arg2)); \
  asm ("movq %0, %%rax" : : "i"(num));  \
  asm ("syscall; movl %0, %%eax" : "=r"(result));

#define SYSCALL_3(num, arg1, arg2, arg3)  \
  asm ("mov %0, %%rdi" : : "r"(arg1));   \
  asm ("mov %0, %%rsi" : : "r"(arg2));   \
  asm ("mov %0, %%rdx" : : "r"(arg3));   \
  asm ("mov %0, %%rax" : : "i"(num));    \
  asm ("syscall; movl %0, %%eax" : "=r"(result));

int syscall_write(unsigned int fd, const char* buf, size_t count)
{
  int result;
  SYSCALL_3(SYS_WRITE, fd, buf, count);
  return result;
}
