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

#include <hw/apic_timer.hpp>
#include <hw/apic_regs.hpp>
#include <hw/pit.hpp>
#include <kernel/irq_manager.hpp>
#include <cstdio>
#include <info>

#define TIMER_ONESHOT     0x0
#define TIMER_PERIODIC    0x20000
#define TIMER_DEADLINE    0x40000

// vol 3a  10-10
#define DIVIDE_BY_16     0x3

using namespace std::chrono;

namespace hw
{
  static delegate<void()> intr_handler;
  static uint32_t         ticks_per_micro = 0;
  
  void APIC_Timer::init()
  {
    // decrement only every 16 ticks
    lapic.regs->divider_config.reg = DIVIDE_BY_16;
    // start in one-shot mode and set the interrupt vector
    // but also disable interrupts
    lapic.regs->timer.reg = TIMER_ONESHOT | (LAPIC_IRQ_TIMER+32) | INTR_MASK;
    
    // start timer (unmask)
    INFO("APIC", "Measuring APIC timer...");
    lapic.regs->init_count.reg = 0xFFFFFFFF;
    // 0xFFFFFFFF --> ~68 seconds
    // 0xFFFFFF   --> ~46 milliseconds
    
    // See: Vol3a 10.5.4.1 TSC-Deadline Mode
    
    /// use PIT to measure <time> in one-shot ///
    hw::PIT::instance().on_timeout_ms(milliseconds(10),
    [] {
      // measure difference
      uint32_t diff = lapic.regs->init_count.reg - lapic.regs->cur_count.reg;
      ticks_per_micro = diff / 10 / 1000;
      
      printf("* APIC timer: ticks 10ms: %u\t 1micro: %u\n", 
            diff, ticks_per_micro);
      
      // make sure timer is still disabled
      lapic.regs->timer.reg |= INTR_MASK;
      
      // enable normal timer functionality
      IRQ_manager::cpu(0).subscribe(LAPIC_IRQ_TIMER, intr_handler);
    });
  }
  
  bool APIC_Timer::ready()
  {
    return ticks_per_micro != 0;
  }
  
  void APIC_Timer::set_handler(delegate<void()>& handler)
  {
    intr_handler = handler;
    if (ready()) {
      IRQ_manager::cpu(0).subscribe(LAPIC_IRQ_TIMER, handler);
    }
  }
  
  void APIC_Timer::oneshot(uint32_t microsec)
  {
    // set initial counter
    lapic.regs->init_count.reg = microsec * ticks_per_micro;
    // enable interrupt vector
    lapic.regs->timer.reg = (lapic.regs->timer.reg & ~INTR_MASK) | TIMER_ONESHOT;
    
  }
  void APIC_Timer::stop()
  {
    lapic.regs->timer.reg |= INTR_MASK;
  }
}
