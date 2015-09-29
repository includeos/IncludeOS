//#define DEBUG
#include <hw/cpu_freq_sampling.hpp>
#include <common>
#include <vector>
#include <algorithm>
#include <os>
#include <class_irq_handler.hpp>

/** @note C-style code here, since we're dealing with interrupt handling. 
    The hardware expects a pure function pointer, and asm can't (easily) 
    call class member functions.  */

static uint32_t sample_counter_ = 0;

// The PIT-chip runs at this fixed frequency (in MHz) , according to OSDev.org
static constexpr double freq_mhz_ = 14.31818 / 12;   // ~ 1.1931816666666666 MHz
extern "C" constexpr double ticks_pr_sec_ = freq_mhz_ * 1'000'000;   // ~ 1.1931816666666

extern "C" double _CPUFreq_ = 0;
extern "C" uint64_t _cpu_prev_timestamp_ = 0;
extern "C" uint16_t _cpu_sampling_freq_divider_  = freq_mhz_ * 1000;

static constexpr int do_samples_ = 10;

std::vector<double>samples;

void cpu_sampling_irq_handler(){

  auto t2 = OS::cycles_since_boot();

  sample_counter_++;
  
  
  // Skip first couple of (3) samples
  if (sample_counter_ < 3){
    IRQ_handler::eoi(0);
    return;
  }   
 
  uint64_t cycles =  t2 - _cpu_prev_timestamp_;
  assert(cycles < (uint32_t)-1);
  
  _cpu_prev_timestamp_ = t2;
  
  double adjusted_ticks_pr_sec = _cpu_sampling_freq_divider_ == 0 ? ticks_pr_sec_ / 0xffff : ticks_pr_sec_ / _cpu_sampling_freq_divider_;  
  double sec_between_ticks = 1 / adjusted_ticks_pr_sec;
  
  
  double freq = (cycles / sec_between_ticks) / 1'000'000;

  if (freq > 1000 && freq < 10000)
    samples.push_back(freq);
  
  if (sample_counter_ >= do_samples_ + 3 and freq > 10){
    double sum = 0;
    
    for(int i = 5; i<samples.size(); i++){
      sum += samples[i];
      //debug("\t Sample: %f \n",samples[i]);
    }
    
    std::sort(samples.begin(),samples.end());
    
    double avg = sum / (samples.size() - 5);
    double median = samples[samples.size() / 2];
    double median_avg = (samples[(samples.size() / 2)-1] + median + samples[(samples.size() / 2) +1]) / 3;
    debug ("<PIT CPU Freq.Sampler> AVERAGE Freq.: %f  MEDIAN Freq.: %f MEDIAN/AVG: %f \n",
	   avg, median, median_avg );
    _CPUFreq_ = median;
    IRQ_handler::eoi(0);
    return;
  }

  
  debug("<PIT CPU Freq. Sampler> Sample %i. tics_pr_sec: %f  adjusted_ticks_pr_sec: %f sec_between_ticks: %f divider: %i \n", 
	sample_counter_, ticks_pr_sec_, adjusted_ticks_pr_sec, sec_between_ticks, _cpu_sampling_freq_divider_);
  debug("<PIT CPU Freq. Sampler> Cycles  %u in %f sec., Freq: %f MHz\n", (uint32_t)cycles, sec_between_ticks, freq); 
  
  
  IRQ_handler::eoi(0);
    
}
