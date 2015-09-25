#define DEBUG 
#include <os>
#include <class_pit.hpp>
#include <class_irq_handler.hpp>
#include <hw/cpu_freq_sampling.hpp>

// Bit 0-3: Mode 0 - "Interrupt on terminal count"
// Bit 4-5: Both set, access mode "Lobyte / Hibyte"
const uint8_t PIT_one_shot = 0x30;  
const uint8_t PIT_mode_register = 0x43;
const uint8_t  PIT_chan0 = 0x40;

// Private, singular instance
PIT* PIT::pit_ = 0;

PIT::Mode PIT::current_mode_ = NONE;
PIT::Mode PIT::temp_mode_ = NONE;

uint64_t PIT::IRQ_counter_ = 0;

uint64_t PIT::prev_timestamp_ = 0;

extern "C" double _CPUFreq_;

uint16_t PIT::current_freq_divider_ = 0;
uint16_t PIT::temp_freq_divider_ = 0;
uint16_t PIT::sampling_freq_divider_ = 0xffff;



void PIT::disable_regular_interrupts()
{ 
  
  // Set one-shot mode, without setting a frequency
  //set_mode(ONE_SHOT);

}

PIT::~PIT(){
  delete pit_;
}

PIT::PIT(){
  debug("<PIT> Instantiating. \n");
}


void PIT::estimateCPUFrequency(){

  debug("<PIT EstimateCPUFreq> Saving state\n");
  // Save PIT-state
  temp_mode_ = current_mode_;
  temp_freq_divider_ = current_freq_divider_;
  
  auto prev_irq_handler = IRQ_handler::get_handler(32);

  // Initialize CPU sampling. This 
  _CPUFreq_ = 0;  

  
  debug("<PIT EstimateCPUFreq> Sampling\n");
  IRQ_handler::set_handler(32, cpu_sampling_irq_entry);

  // GO!
  set_mode(RATE_GEN);  
  set_freq(sampling_freq_divider_); 
  
  while (_CPUFreq_ == 0) {
    debug("<PIT EstimateCPUFreq> CPUFreq_ is %i \n", _CPUFreq_);
    OS::halt();
  }
    
  debug("<PIT EstimateCPUFreq> Done. Reult: %f \n", _CPUFreq_);
  

  // Reset the PIT to previous state  
  set_mode(temp_mode_);
  set_freq(temp_freq_divider_);
  IRQ_handler::set_handler(32, prev_irq_handler);

}

double PIT::CPUFrequency(){
  if (! _CPUFreq_)
    estimateCPUFrequency();
  
  return _CPUFreq_;
}

void PIT::onRepeatedTimeout_sec(uint64_t ms, timeout_handler){
  debug("<PIT sec> SORRY, timers aren't done yet \n");
};

void PIT::onRepeatedTimeout_ms(uint32_t sec, timeout_handler){
  debug("<PIT ms> SORRY, timers aren't done yet \n");
};
  

void PIT::irq_handler(){

  IRQ_counter_ ++;

  uint16_t freq_devider =  0x0;
  double adjusted_ticks_pr_sec = freq_devider == 0 ? ticks_pr_sec_ / 0xffff : ticks_pr_sec_ / freq_devider;  
  double time_between_ticks = 1 / adjusted_ticks_pr_sec;
  
  OS::rsprint("#");
  if (IRQ_counter_ % (int)adjusted_ticks_pr_sec == 0) {
    debug("<PIT> One-shot ordered, with t= %i expected in %f seconds. Current ticks pr. sec: %f \n", 
	  freq_devider, time_between_ticks, adjusted_ticks_pr_sec );
  }
  //oneshot(freq_devider);
  IRQ_handler::eoi(0);
}

void PIT::init(){
  debug("<PIT> Initializing @ frequency: %16.16f MHz. Assigning myself to all timer interrupts.\n ", freq_mhz_);  
  PIT::disable_regular_interrupts();
  IRQ_handler::enable_irq(32);
  IRQ_handler::subscribe(0, irq_handler);
}

void PIT::set_mode(Mode mode){
  
  if (mode == current_mode_){
    debug("<PIT::set_mode> Allready set. IGNOING \n");
    return;
    
  }


  debug("<PIT::set_mode> Setting mode %#x \n", mode);
  // Channel is the last two bits in the PIT mode register
  // ...we always use channel 0
  auto channel = 0x00;
  
  uint8_t config = mode | LO_HI | channel;
  OS::outb(PIT_mode_register, config);
  current_mode_ = mode;

}



void PIT::set_freq(uint16_t freq_devider){
  
  if (freq_devider == current_freq_divider_)
    return;
  
  union{
    uint16_t whole;
    uint8_t part[2];
  }data{freq_devider};
  
  // Set a frequency 
  OS::outb(PIT_chan0, data.part[0]);
  OS::outb(PIT_chan0, data.part[1]);
  
}


void PIT::oneshot(uint16_t t){
    
  // Enable 1-shot mode
  set_mode(ONE_SHOT);  
  
  // Set a frequency for shot
  set_freq(t);
}

