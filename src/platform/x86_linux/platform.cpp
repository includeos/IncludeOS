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

#include <kernel/events.hpp>
#include <kernel/os.hpp>
#include <info>
#define MYINFO(X,...) INFO("x86", X, ##__VA_ARGS__)

extern "C" char* get_cpu_esp();
extern "C" void* get_cpu_ebp();
#define _SENTINEL_VALUE_   0x123456789ABCDEF

namespace tls {
  extern size_t get_tls_size();
  extern void   fill_tls_data(char*);
}
struct alignas(64) smp_table
{
  // thread self-pointer
  void* tls_data; // 0x0
  // per-cpu cpuid (and more)
  int cpuid;
  int reserved;

#ifdef ARCH_x86_64
  uintptr_t pad[3]; // 64-bit padding
  uintptr_t guard; // _SENTINEL_VALUE_
#else
  uint32_t  pad[2];
  uintptr_t guard; // _SENTINEL_VALUE_
#endif
  /** put more here **/
};
#ifdef ARCH_x86_64
// FS:0x28 on Linux is storing a special sentinel stack-guard value
static_assert(offsetof(smp_table, guard) == 0x28, "Linux stack sentinel");
#endif

void __platform_init()
{
  INFO("Linux", "Initialize event manager");
  Events::get(0).init_local();

}

void __arch_enable_legacy_irq(uint8_t) {}
void __arch_disable_legacy_irq(uint8_t) {}

void __arch_poweroff()
{
  // exit(0) syscall
  __builtin_unreachable();
}
void __arch_reboot()
{
  // exit(0) syscall
  __builtin_unreachable();
}
