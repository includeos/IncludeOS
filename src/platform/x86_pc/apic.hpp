
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
