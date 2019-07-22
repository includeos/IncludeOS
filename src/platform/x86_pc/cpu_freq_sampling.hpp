// 
// 

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
