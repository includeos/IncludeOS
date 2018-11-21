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

#include <hal/machine.hpp>

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

  
  /** Block for a while, e.g. until the next round in the event loop **/
  void block();
  
  /**
   *  Halt until next event.
   *
   *  @Warning If there is no regular timer interrupt (i.e. from PIT / APIC)
   *  we'll stay asleep.
   */
  void halt();

  /** Full system reboot **/
  void reboot();

  /** Power off system **/
  void shutdown();
  
  /** Clock cycles since boot. */
  inline uint64_t cycles_since_boot() noexcept;

  /** Nanoseconds since boot converted from cycles */
  inline uint64_t nanos_since_boot() noexcept; 

  /** Time spent sleeping (halt) in cycles */
  uint64_t cycles_asleep() noexcept;

  /** Time spent sleeping (halt) in nanoseconds */
  uint64_t nanos_asleep() noexcept;

  
}

// Inline implementation details
#include <detail/os.hpp>


#endif // OS_HPP
