// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

/*
 * Self sufficient Utility function that will copy a binary to a certain 
 * location and then jump to it. The idea (from fwsgonzo) is to have this 
 * function copied to an otherwise unused place in memory so that we can
 * overwrite the currently running binary with a new one.
 */

asm(".org 0x8000");
#define BOOT_MAGIC   0x1

extern "C" __attribute__((noreturn))
void hotswap(const char* base, int len, char* dest, void* start)
{
  // replace old kernel with new
  for (int i = 0; i < len; i++)
    dest[i] = base[i];
  
  // jump to _start
  asm volatile("jmp *%1" : : "a" (BOOT_MAGIC), "b" (start) : "eax", "ebx");

  // we can never get here!
  __builtin_unreachable();
}
