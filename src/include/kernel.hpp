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

#ifndef KERNEL_HPP
#define KERNEL_HPP

#include <hal/machine.hpp>
#include <util/units.hpp>

namespace kernel {

  struct State {
    bool running               = true;
    bool boot_sequence_passed  = false;
    bool libc_initialized      = false;
    bool block_drivers_ready   = false;
    bool timestamps            = false;
    bool timestamps_ready      = false;
    bool is_live_updated       = false;
    int  panics                = 0;
    const char* cmdline        = nullptr;
    util::KHz cpu_khz {-1};
  };

  State& state() noexcept;

  inline bool is_running() noexcept {
    return state().running;
  }

  inline bool is_booted() noexcept {
    return state().boot_sequence_passed;
  }

  inline bool libc_initialized() noexcept {
    return state().libc_initialized;
  }

  inline bool block_drivers_ready() noexcept {
    return state().block_drivers_ready;
  };

  inline bool timestamps() noexcept {
    return state().timestamps;
  }

  inline bool timestamps_ready() noexcept {
    return state().timestamps_ready;
  }

  inline bool is_live_updated() noexcept {
    return state().is_live_updated;
  }

  inline bool is_panicking() {
    return state().panics > 0;
  };

  inline int panics() {
    return state().panics;
  }

  inline const char* cmdline() {
    return state().cmdline;
  };


  using ctor_t = void (*)();
  inline static void run_ctors(ctor_t* begin, ctor_t* end)
  {
  	for (; begin < end; begin++) (*begin)();
  }

  inline util::KHz cpu_freq() {
    return state().cpu_khz;
  }

  bool heap_ready();
  void init_heap(os::Machine::Memory& mem);
}

#endif
