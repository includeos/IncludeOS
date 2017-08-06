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
#ifndef X86_CPU_FREQ_SAMPLING_HPP
#define X86_CPU_FREQ_SAMPLING_HPP

namespace x86
{
  static const int CPU_FREQUENCY_SAMPLES = 18;

  extern void   reset_cpufreq_sampling();
  extern double calculate_cpu_frequency();

  /** Proper IRQ-handler for CPU frequency sampling - implemented in interrupts.s
      @Note 
      PIT::estimateCPUFrequency() will register- /de-register this as needed */
  extern "C" void cpu_sampling_irq_handler();

  /** CPU frequency sampling. Implemented in hw/cpu_freq_sampling.cpp 
      @Note this will be automatically called by the oirq-handler  */
  extern "C" void cpu_sampling_irq_entry();

} //< namespace hw

#endif
