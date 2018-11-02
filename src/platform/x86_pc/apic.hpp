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
#ifndef HW_APIC_HPP
#define HW_APIC_HPP

#include <cstdint>
#include <delegate>
#include "xapic.hpp"
#include "x2apic.hpp"

namespace x86
{
  class APIC {
  public:
    static IApic& get() noexcept;

    // enable and disable legacy IRQs
    static void enable_irq (uint8_t irq);
    static void disable_irq(uint8_t irq);

    static int get_isr() noexcept {
      return get().get_isr();
    }
    static int get_irr() noexcept {
      return get().get_irr();
    }
    static void eoi() noexcept {
      return get().eoi();
    }

    static void init();
  };
}

#endif
