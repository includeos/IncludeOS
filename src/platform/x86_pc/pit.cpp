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

#include "pit.hpp"
#include "cpu_freq_sampling.hpp"
#include <hw/ioport.hpp>
#include <os.hpp>
#include <kernel/events.hpp>
#include <kernel/rtc.hpp>
//#undef NO_DEBUG
#define DEBUG
#define DEBUG2

// Used for cpu frequency sampling
extern const uint16_t _cpu_sampling_freq_divider_;
using namespace std::chrono;

// Used by blocking_cycle() to block until one interrupt happens
extern "C" void blocking_cycle_irq_entry();
static int blocking_cycle_done = 0;
extern "C"
void blocking_cycle_irq_handler() {
  blocking_cycle_done++;
}

namespace x86
{
  constexpr MHz PIT::FREQUENCY;

  // Bit 0-3: Mode 0 - "Interrupt on terminal count"
  // Bit 4-5: Both set, access mode "Lobyte / Hibyte"
  const uint8_t PIT_mode_register = 0x43;
  const uint8_t PIT_chan0 = 0x40;

  void PIT::disable_regular_interrupts()
  {
    if (current_mode_ != ONE_SHOT)
        enable_oneshot(1);
  }

  double PIT::estimate_CPU_frequency()
  {
    debug("<CPU frequency> Saving state: curr_freq_div %i \n", current_freq_divider_);
    reset_cpufreq_sampling();

    // Save PIT-state
    auto temp_mode     = get().current_mode_;
    auto temp_freq_div = get().current_freq_divider_;

    debug("<CPU frequency> Measuring...\n");
    __arch_install_irq(0, cpu_sampling_irq_entry);

    // GO!
    get().set_mode(RATE_GEN);
    get().set_freq_divider(_cpu_sampling_freq_divider_);

    // BLOCKING call to external measurment.
    double freq = calculate_cpu_frequency();

    debug("<CPU frequency> Result: %f hz\n", freq);

    get().set_mode(temp_mode);
    get().set_freq_divider(temp_freq_div);

    __arch_subscribe_irq(0);
    return freq;
  }

  void PIT::blocking_cycles(int total)
  {
    // Save PIT-state
    auto temp_mode     = get().current_mode_;
    auto temp_freq_div = get().current_freq_divider_;

    blocking_cycle_done = 0;
    __arch_install_irq(0, blocking_cycle_irq_entry);
    // GO!
    get().set_mode(RATE_GEN);
    get().set_freq_divider(MILLISEC_INTERVAL);

    while (blocking_cycle_done < total) {
      asm("hlt");
    }

    get().set_mode(temp_mode);
    get().set_freq_divider(temp_freq_div);
    __arch_subscribe_irq(0);
  }

  static inline auto now() noexcept
  {
    return duration_cast<nanoseconds> (nanoseconds(RTC::nanos_now()));
  }

  void PIT::oneshot(milliseconds timeval, timeout_handler handler)
  {
    if (get().current_mode_ != RATE_GEN)
      get().set_mode(RATE_GEN);
    if (get().current_freq_divider_ != MILLISEC_INTERVAL)
      get().set_freq_divider(MILLISEC_INTERVAL);

    bool forever = timeval == milliseconds::zero();
    if (forever) {
      get().forev_handler = handler;
    } else {
      get().expiration    = now() + duration_cast<nanoseconds>(timeval);
      get().handler       = handler;
    }
  }

  void PIT::forever(timeout_handler handler)
  {
    oneshot(milliseconds::zero(), handler);
  }

  uint8_t PIT::read_back()
  {
    const uint8_t READ_BACK_CMD = 0xc2;

    hw::outb(PIT_mode_register, READ_BACK_CMD);
    auto res = hw::inb(PIT_chan0);

    debug("STATUS: %#x \n", res);
    return res;
  }

  void PIT::irq_handler()
  {
    if (now() >= this->expiration)
    {
      if (this->handler) {
        this->handler();
        this->handler = nullptr;
      }
      // stop PIT when not running forever
      if (this->forev_handler == nullptr)
          disable_regular_interrupts();
    }
    // always call forever handler, if set
    if (this->forev_handler)
    {
      this->forev_handler();
    }
  }

  PIT::PIT()
  {
    debug("<PIT> Initializing @ frequency: %f MHz.\n ", frequency().count());
    PIT::disable_regular_interrupts();
    // must be done to program IOAPIC to redirect to BSP LAPIC
    __arch_enable_legacy_irq(0);
    // register irq handler
    Events::get().subscribe(0, {this, &PIT::irq_handler});
  }

  void PIT::set_mode(Mode mode)
  {
    // Channel is the last two bits in the PIT mode register
    // ...we always use channel 0
    auto channel = 0x00;
    uint8_t config = mode | LO_HI | channel;
    debug("<PIT::set_mode> Setting mode %#x, config: %#x \n", mode, config);

    hw::outb(PIT_mode_register, config);
    current_mode_ = mode;
  }

  void PIT::set_freq_divider(uint16_t value)
  {
    // Send frequency hi/lo to PIT
    hw::outb(PIT_chan0, value & 0xff);
    hw::outb(PIT_chan0, value >> 8);

    current_freq_divider_ = value;
  }

  void PIT::enable_oneshot(uint16_t t)
  {
    // Enable 1-shot mode
    set_mode(ONE_SHOT);

    // Set a frequency for shot
    set_freq_divider(t);
  }

} //< x86

void __arch_preempt_forever(void(*func)())
{
  x86::PIT::forever(func);
}
