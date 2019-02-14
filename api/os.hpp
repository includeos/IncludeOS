// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

#ifndef OS_HPP
#define OS_HPP

//#include <hal/machine.hpp>
#include <cstdint>
#include <cstddef>
#include <delegate>
#include <gsl/gsl>

namespace os {

  /** @returns the name of the CPU architecture for which the OS was built */
  const char* arch() noexcept;

  /** @returns IncludeOS verison identifier */
  const char* version() noexcept;

  /**
   * Returns the parameters passed, to the system at boot time.
   * The first argument is always the binary name.
   **/
  const char* cmdline_args() noexcept;

  //
  // Control flow
  //

  /** Block for a while, e.g. until the next round in the event loop **/
  void block() noexcept;

  /** Enter the main event loop.  Trigger subscribed and triggerd events */
  void event_loop();


  /**
   *  Halt until next event.
   *
   *  @Warning If there is no regular timer interrupt (i.e. from PIT / APIC)
   *  we'll stay asleep.
   */
  void halt() noexcept;

  /** Full system reboot **/
  void reboot() noexcept;

  /** Power off system **/
  void shutdown() noexcept;


  //
  // Time
  //

  /** Clock cycles since boot. **/
  inline uint64_t cycles_since_boot() noexcept;

  /** Nanoseconds since boot converted from cycles **/
  inline uint64_t nanos_since_boot() noexcept;

  /** Time spent sleeping (halt) in cycles **/
  uint64_t cycles_asleep() noexcept;

  /** Time spent sleeping (halt) in nanoseconds **/
  uint64_t nanos_asleep() noexcept;


  //
  // Panic
  //

  /** Trigger unrecoverable error and output diagnostics **/
  __attribute__((noreturn))
  void panic(const char* why) noexcept;

  /** Default behavior after panic **/
  enum class Panic_action {
    halt, reboot, shutdown
  };


  Panic_action panic_action() noexcept;
  void set_panic_action(Panic_action action) noexcept;

  typedef void (*on_panic_func) (const char*);

  /** The on_panic handler will be called directly after a panic if possible.**/
  void on_panic(on_panic_func);

  //using Plugin      = delegate<void()>;
  //using Span_mods   = gsl::span<multiboot_module_t>;


  //
  // Print
  //

  using print_func  = delegate<void(const char*, size_t)>;

  /**  Write data to standard out callbacks **/
  void print(const char* ptr, const size_t len);

  /** Add handler for standard output **/
  void add_stdout(print_func func);

  /** The OS default stdout handler. TODO: try to remove me */
  void default_stdout(const char*, size_t);

  /**
   *  Enable or disable automatic prepension of
   *  timestamps to all os::print calls
   */
  void print_timestamps(bool enabled);

  /** Print current call stack **/
  void print_backtrace() noexcept;

  /** Print current callstack using provided print function */
  void print_backtrace(void(*print_func)(const char*, size_t)) noexcept;


  //
  // Memory
  //

  /** Total used memory, including reserved areas */
  size_t total_memuse() noexcept;



  //
  // Kernel modules, plugins
  //
  struct Module {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t params;
    uint32_t pad;
  };

  using Span_mods = gsl::span<Module>;
  Span_mods modules();

  using Plugin = delegate<void()>;
  /**
   * Register a custom initialization function. The provided delegate is
   * guaranteed to be called after global constructors and device initialization
   * and before Service::start, provided that this funciton was called by a
   * global constructor.
   * @param delg : A delegate to be called
   * @param name : A human readable identifier
  **/
  void register_plugin(Plugin delg, const char* name);


  //
  // HAL - portable hardware representation
  //

  class Machine;
  Machine& machine() noexcept;


}

// Inline implementation details
#include <detail/os.hpp>


#endif // OS_HPP
