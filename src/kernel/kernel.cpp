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

#include <os.hpp>
#include <kernel.hpp>
#include <kernel/cpuid.hpp>
#include <kernel/rng.hpp>
#include <service>
#include <cstdio>
#include <cinttypes>
#include <util/fixed_vector.hpp>
#include <system_log>
#define MYINFO(X,...) INFO("Kernel", X, ##__VA_ARGS__)

//#define ENABLE_PROFILERS
#ifdef ENABLE_PROFILERS
#include <profile>
#define PROFILE(name)  ScopedProfiler __CONCAT(sp, __COUNTER__){name};
#else
#define PROFILE(name) /* name */
#endif

using namespace util;

extern char _start;
extern char _end;
extern char _ELF_START_;
extern char _TEXT_START_;
extern char _LOAD_START_;
extern char _ELF_END_;

//uintptr_t OS::liveupdate_loc_   = 0;

kernel::State __kern_state;

kernel::State& kernel::state() noexcept {
  return __kern_state;
}

util::KHz os::cpu_freq() {
  return kernel::cpu_freq();
}

// stdout redirection
using Print_vec = Fixed_vector<os::print_func, 8>;
static Print_vec os_print_handlers(Fixedvector_Init::UNINIT);

// Plugins
struct Plugin_desc {
  Plugin_desc(os::Plugin f, const char* n) : func{f}, name{n} {}

  os::Plugin  func;
  const char* name;
};
static Fixed_vector<Plugin_desc, 16> plugins(Fixedvector_Init::UNINIT);


__attribute__((weak))
size_t kernel::liveupdate_phys_size(size_t /*phys_max*/) noexcept {
  return 4096;
};

__attribute__((weak))
size_t kernel::liveupdate_phys_loc(size_t phys_max) noexcept {
  return phys_max - liveupdate_phys_size(phys_max);
};

__attribute__((weak))
void kernel::setup_liveupdate(uintptr_t)
{
  // without LiveUpdate: storage location is at the last page?
  kernel::state().liveupdate_loc = kernel::heap_max() & ~(uintptr_t) 0xFFF;
}

const char* os::cmdline_args() noexcept {
  return kernel::cmdline();
}

extern kernel::ctor_t __plugin_ctors_start;
extern kernel::ctor_t __plugin_ctors_end;
extern kernel::ctor_t __service_ctors_start;
extern kernel::ctor_t __service_ctors_end;

void os::register_plugin(Plugin delg, const char* name){
  MYINFO("Registering plugin %s", name);
  plugins.emplace_back(delg, name);
}

extern void __arch_reboot();
void os::reboot() noexcept
{
  __arch_reboot();
}
void os::shutdown() noexcept
{
  kernel::state().running = false;
}

void kernel::post_start()
{
  // Enable timestamps (if present)
  kernel::state().timestamps_ready = true;

  // LiveUpdate needs some initialization, although only if present
  kernel::setup_liveupdate();

  // Initialize the system log if plugin is present.
  // Dependent on the liveupdate location being set
  SystemLog::initialize();

  MYINFO("Initializing RNG");
  PROFILE("RNG init");
  RNG::get().init();

  // Seed rand with 32 bits from RNG
  srand(rng_extract_uint32());

#ifndef __MACH__
  // Custom initialization functions
  MYINFO("Initializing plugins");
  kernel::run_ctors(&__plugin_ctors_start, &__plugin_ctors_end);
#endif

  // Run plugins
  PROFILE("Plugins init");
  for (auto plugin : plugins) {
    INFO2("* Initializing %s", plugin.name);
    plugin.func();
  }

  MYINFO("Running service constructors");
  FILLINE('-');
  // the boot sequence is over when we get to plugins/Service::start
  kernel::state().boot_sequence_passed = true;

#ifndef __MACH__
    // Run service constructors
  kernel::run_ctors(&__service_ctors_start, &__service_ctors_end);
#endif

  PROFILE("Service::start");
  // begin service start
  FILLINE('=');
  printf(" IncludeOS %s (%s / %u-bit)\n",
         os::version(), os::arch(),
         static_cast<unsigned>(sizeof(uintptr_t)) * 8);
  printf(" +--> Running [ %s ]\n", Service::name());
  FILLINE('~');

  // if we have disabled important checks, its unsafe for production
#if defined(LIBFUZZER_ENABLED) || defined(ARP_PASSTHROUGH) || defined(DISABLE_INET_CHECKSUMS)
  const bool unsafe = true;
#else
  // if we dont have a good random source, its unsafe for production
  const bool unsafe = CPUID::has_feature(CPUID::Feature::RDRAND);
#endif
  if (unsafe) {
    printf(" +--> WARNiNG: Environment unsafe for production\n");
    FILLINE('~');
  }

  // service program start
  Service::start();
}

void os::add_stdout(os::print_func func)
{
  os_print_handlers.push_back(func);
}

void os::default_stdout(const char* str, size_t len)
{
  kernel::default_stdout(str, len);
}
__attribute__((weak))
bool os_enable_boot_logging = false;
__attribute__((weak))
bool os_default_stdout = false;

#include <isotime>
static inline bool contains(const char* str, size_t len, char c)
{
  for (size_t i = 0; i < len; i++) if (str[i] == c) return true;
  return false;
}

void os::print(const char* str, const size_t len)
{
  if (UNLIKELY(! kernel::libc_initialized())) {
    kernel::default_stdout(str, len);
    return;
  }

  /** TIMESTAMPING **/
  if (kernel::timestamps() && kernel::timestamps_ready() && !kernel::is_panicking())
  {
    static bool apply_ts = true;
    if (apply_ts)
    {
      std::string ts = "[" + isotime::now() + "] ";
      for (const auto& callback : os_print_handlers) {
        callback(ts.c_str(), ts.size());
      }
      apply_ts = false;
    }
    const bool has_newline = contains(str, len, '\n');
    if (has_newline) apply_ts = true;
  }
  /** TIMESTAMPING **/

  if (os_enable_boot_logging || kernel::is_booted() || kernel::is_panicking())
  {
    for (const auto& callback : os_print_handlers) {
      callback(str, len);
    }
  }
}

void os::print_timestamps(const bool enabled)
{
  kernel::state().timestamps = enabled;
}
