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

//#define DEBUG
#define MYINFO(X,...) INFO("Kernel", X, ##__VA_ARGS__)

#ifndef PANIC_ACTION
#define PANIC_ACTION halt
#endif

#include <kernel/os.hpp>
#include <kernel/rng.hpp>
#include <service>
#include <cstdio>
#include <cinttypes>
#include <util/fixed_vector.hpp>
#include <system_log>

//#define ENABLE_PROFILERS
#ifdef ENABLE_PROFILERS
#include <profile>
#define PROFILE(name)  ScopedProfiler __CONCAT(sp, __COUNTER__){name};
#else
#define PROFILE(name) /* name */
#endif

using namespace util;

extern "C" void* get_cpu_esp();
extern uintptr_t _start;
extern uintptr_t _end;
extern uintptr_t _ELF_START_;
extern uintptr_t _TEXT_START_;
extern uintptr_t _LOAD_START_;
extern uintptr_t _ELF_END_;

bool __libc_initialized = false;

bool  OS::power_   = true;
bool  OS::boot_sequence_passed_ = false;
bool  OS::m_is_live_updated     = false;
bool  OS::m_block_drivers_ready = false;
KHz   OS::cpu_khz_ {-1};
uintptr_t OS::liveupdate_loc_   = 0;

OS::Panic_action OS::panic_action_ = OS::Panic_action::PANIC_ACTION;
const uintptr_t OS::elf_binary_size_ {(uintptr_t)&_ELF_END_ - (uintptr_t)&_ELF_START_};

// stdout redirection
using Print_vec = Fixed_vector<OS::print_func, 8>;
static Print_vec os_print_handlers(Fixedvector_Init::UNINIT);

// Plugins
struct Plugin_desc {
  Plugin_desc(OS::Plugin f, const char* n) : func{f}, name{n} {}

  OS::Plugin  func;
  const char* name;
};
static Fixed_vector<Plugin_desc, 16> plugins(Fixedvector_Init::UNINIT);

// OS version
std::string OS::version_str_ = OS_VERSION;
std::string OS::arch_str_ = ARCH;

void* OS::liveupdate_storage_area() noexcept
{
  return (void*) OS::liveupdate_loc_;
}

__attribute__((weak))
void OS::setup_liveupdate(uintptr_t)
{
  // without LiveUpdate: storage location is at the last page?
  OS::liveupdate_loc_ = OS::heap_max() & ~(uintptr_t) 0xFFF;
}

const char* OS::cmdline = nullptr;
const char* OS::cmdline_args() noexcept {
  return cmdline;
}

typedef void (*ctor_t) ();
extern ctor_t __service_ctors_start;
extern ctor_t __service_ctors_end;
extern ctor_t __plugin_ctors_start;
extern ctor_t __plugin_ctors_end;

int __run_ctors(ctor_t* begin, ctor_t* end)
{
  int i = 0;
	for (; begin < end; begin++, i++) (*begin)();
  return i;
}

void OS::register_plugin(Plugin delg, const char* name){
  MYINFO("Registering plugin %s", name);
  plugins.emplace_back(delg, name);
}

void OS::reboot()
{
  extern void __arch_reboot();
  __arch_reboot();
}
void OS::shutdown()
{
  MYINFO("Soft shutdown signalled");
  power_ = false;
}

void OS::post_start()
{
  // LiveUpdate needs some initialization, although only if present
  OS::setup_liveupdate();

  // Initialize the system log if plugin is present.
  // Dependent on the liveupdate location being set
  SystemLog::initialize();

  MYINFO("Initializing RNG");
  PROFILE("RNG init");
  RNG::get().init();

  // Seed rand with 32 bits from RNG
  srand(rng_extract_uint32());

  // Custom initialization functions
  MYINFO("Initializing plugins");

  // Run plugin constructors
  __run_ctors(&__plugin_ctors_start, &__plugin_ctors_end);

  // Run plugins
  PROFILE("Plugins init");
  for (auto plugin : plugins) {
    INFO2("* Initializing %s", plugin.name);
    plugin.func();
  }

  MYINFO("Running service constructors");
  FILLINE('-');
  // the boot sequence is over when we get to plugins/Service::start
  OS::boot_sequence_passed_ = true;

    // Run service constructors
  __run_ctors(&__service_ctors_start, &__service_ctors_end);

  PROFILE("Service::start");
  // begin service start
  FILLINE('=');
  printf(" IncludeOS %s (%s / %i-bit)\n",
         version().c_str(), arch().c_str(),
         static_cast<int>(sizeof(uintptr_t)) * 8);
  printf(" +--> Running [ %s ]\n", Service::name());
  FILLINE('~');

  Service::start();
}

void OS::add_stdout(OS::print_func func)
{
  os_print_handlers.push_back(func);
}
__attribute__((weak))
bool os_enable_boot_logging = false;
__attribute__((weak))
bool os_default_stdout = false;

void OS::print(const char* str, const size_t len)
{
  if (UNLIKELY(! __libc_initialized)) {
    OS::default_stdout(str, len);
    return;
  }

  for (auto& callback : os_print_handlers) {
    if (os_enable_boot_logging || OS::is_booted() || OS::is_panicking())
      callback(str, len);
  }
}
