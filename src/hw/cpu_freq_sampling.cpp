#define DEBUG
#include <hw/cpu_freq_sampling.hpp>
#include <common>
#include <os>
#include <class_irq_handler.hpp>

/** @note C-style code here, since we're dealing with interrupt handling. 
    The hardware expects a pure function pointer, and asm can't (easily) 
    call class member functions.  */

static uint32_t sample_counter_ = 0;

// The PIT-chip runs at this fixed frequency (in MHz) , according to OSDev.org
static constexpr double freq_mhz_ = 14.31818 / 12;   // ~ 1.1931816666666666 MHz
static constexpr double ticks_pr_sec_ = freq_mhz_ * 1'000'000;   // ~ 1.1931816666666

extern "C" double _CPUFreq_ = 0;
extern "C" uint64_t _cpu_prev_timestamp_ = 0;
extern "C" uint64_t _cpu_sampling_freq_divider_ = 0xffff;

static constexpr int do_samples_ = 10;

void cpu_sampling_irq_handler(){

  auto t2 = OS::cycles_since_boot();
  OS::rsprint(".");
  sample_counter_++;
  
  
  // Skip first couple of samples
  if (sample_counter_ < 2){
    IRQ_handler::eoi(0);
    return;
  }   
 
  uint64_t cycles =  t2 - _cpu_prev_timestamp_;
  assert(cycles < (uint32_t)-1);
  
  _cpu_prev_timestamp_ = t2;
  
  double adjusted_ticks_pr_sec = _cpu_sampling_freq_divider_ == 0 ? ticks_pr_sec_ / 0xffff : ticks_pr_sec_ / _cpu_sampling_freq_divider_;  
  double time_between_ticks = 1 / adjusted_ticks_pr_sec;
  
  
  double freq = (cycles * time_between_ticks) / 1'000'000;

  if (sample_counter_ >= do_samples_ + 2){
    _CPUFreq_ = freq;
    IRQ_handler::eoi(0);
    return;
  }


  debug("<PIT CPU Freq. Sampler> Sample %i. PIT freq./Hz: %f  time_window: %f ticks/sec: %f \n", 
	sample_counter_, ticks_pr_sec_, time_between_ticks, adjusted_ticks_pr_sec);
  debug("<PIT CPU Freq. Sampler> Cycles  %u, Freq: %f MHz\n", (uint32_t)cycles, freq); 
  
  
  IRQ_handler::eoi(0);
    
}
