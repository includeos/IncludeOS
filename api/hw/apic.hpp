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
#include <functional>

namespace hw {
  
  class APIC {
  public:
    typedef std::function<void()>  smp_task_func;
    typedef std::function<void()>  smp_done_func;
    
    static void add_task(smp_task_func, smp_done_func);
    static void work_signal();
    
    typedef std::function<void()>   timer_func;
    
    static void init();
    static void setup_subs();
    
    static void send_ipi(uint8_t cpu, uint8_t vector);
    static void send_bsp_intr();
    static void bcast_ipi(uint8_t vector);
    
    // enable and disable legacy IRQs
    static void enable_irq(uint8_t irq);
    static void disable_irq(uint8_t irq);
    
    static uint8_t get_isr();
    static uint8_t get_irr();
    static void eoi();
    
  private:
    static void init_smp();
  };
  
}

#endif
