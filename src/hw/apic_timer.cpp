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

#include <hw/apic.hpp>
#include <hw/apic_regs.hpp>
#include <hw/pit.hpp>
#include <cstdio>

// vol 3a  10-10
#define DIVIDE_BY_16     0x3

namespace hw
{
  void apic_init_timer()
  {
    // divide by 16
    lapic.regs->divider_config.reg = DIVIDE_BY_16;
    // set initial counter
    lapic.regs->init_count.reg = 0xFFFFFFFF;
    // enable timer (unmask)
    lapic.regs->timer.reg = INTR_MASK | LAPIC_IRQ_TIMER;
    
    /// use PIT to measure <time> in one-shot here ///
    
    // stop timer
    lapic.regs->timer.reg &= ~INTR_MASK;
    
    // measure difference
    long diff = lapic.regs->init_count.reg - lapic.regs->cur_count.reg;
    
    // program timer to do useful things ...
    // See: Vol3a 10.5.4.1 TSC-Deadline Mode
  }
  
  void apic_timer_interrupt_handler()
  {
    printf("bang!\n");
    // stop timer
    lapic.regs->timer.reg &= ~INTR_MASK;
  }
}
