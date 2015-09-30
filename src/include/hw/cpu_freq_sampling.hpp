#pragma once
#include <hertz>

/** Proper IRQ-handler for CPU frequency sampling - implemented in interrupts.s
    @Note 
    PIT::estimateCPUFrequency() will register- /de-register this as needed */
extern "C" void cpu_sampling_irq_handler();

/** CPU frequency sampling. Implemented in hw/cpu_freq_sampling.cpp 
    @Note this will be automatically called by the oirq-handler  */
extern "C" void cpu_sampling_irq_entry();

extern "C" void irq_32_entry();

extern "C" MHz calculate_cpu_frequency();
