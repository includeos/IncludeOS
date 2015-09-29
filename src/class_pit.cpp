//#define DEBUG 
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

extern "C" double ticks_pr_sec_;
extern "C" double _CPUFreq_;
extern "C" uint16_t _cpu_sampling_freq_divider_;

uint16_t PIT::current_freq_divider_ = 0;
uint16_t PIT::temp_freq_divider_ = 0;

extern "C" void irq_timer_entry();


void PIT::disable_regular_interrupts()
{   
  oneshot(1);
}

PIT::~PIT(){
  delete pit_;
}

PIT::PIT(){
  debug("<PIT> Instantiating. \n");
}


void PIT::estimateCPUFrequency(){

  debug("<PIT EstimateCPUFreq> Saving state: curr_freq_div %i \n",current_freq_divider_);
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
  set_freq(_cpu_sampling_freq_divider_);   
  
  while (_CPUFreq_ == 0) {
    debug2("<PIT EstimateCPUFreq> CPUFreq_ is %f \n", _CPUFreq_);
    OS::halt();
  }
    
  debug("<PIT EstimateCPUFreq> Done. Result: %f \n", _CPUFreq_);
  
  set_mode(temp_mode_);
  set_freq(temp_freq_divider_);

  IRQ_handler::set_handler(32, prev_irq_handler);

}

double PIT::CPUFrequency(){
  if (! _CPUFreq_)
    estimateCPUFrequency();
  
  return _CPUFreq_;
}

void PIT::onTimeout_ms(uint64_t ms, timeout_handler){
  
  if (ms < 1) panic("Can't wait less than 1 ms. ");
  
  debug("<PIT sec> setting a %i ms. timer \n", ms);
  
  /*
    @Todo
    * Queue the timer
    * Make sure there's an appropriate interrupt coming
  
  if (current_mode_ != RATE_GEN)
    set_mode(RATE_GEN);
  
  if (current_freq_divider_ != freq_mhz_ * 1000)
    set_freq(freq_mhz_ * 1000);
  
  */
  
};

void PIT::onTimeout_sec(uint32_t sec, timeout_handler){
  debug("<PIT sec> setting a %i sec. timer \n", sec);
  
};


uint8_t PIT::read_back(uint8_t channel){
  const uint8_t READ_BACK_CMD = 0xc2;
  
  OS::outb(PIT_mode_register, READ_BACK_CMD );
  
  auto res = OS::inb(PIT_chan0);
  
  debug("STATUS: %#x \n", res);
  
  return res;
  
}

void PIT::irq_handler(){

  IRQ_counter_ ++;

  double adjusted_ticks_pr_sec = current_freq_divider_ == 0 ? 
    ticks_pr_sec_ / 0xffff : ticks_pr_sec_ / current_freq_divider_;  
  
  double time_between_ticks = 1 / adjusted_ticks_pr_sec;

  
  OS::rsprint(".");
  if (IRQ_counter_ % (int)adjusted_ticks_pr_sec == 0) {
    debug("<PIT soft IRQ_handler>  with freq.Div. %i expected in %f seconds. Current ticks pr. sec: %f \n", 
	  current_freq_divider_, time_between_ticks, adjusted_ticks_pr_sec );
  }
  IRQ_handler::eoi(0);

}

void PIT::init(){
  debug("<PIT> Initializing @ frequency: %16.16f MHz. Assigning myself to all timer interrupts.\n ", freq_mhz_);  
  PIT::disable_regular_interrupts();
  //IRQ_handler::enable_irq(32);
  IRQ_handler::subscribe(0, irq_handler);
}

void PIT::set_mode(Mode mode){
  // Channel is the last two bits in the PIT mode register
  // ...we always use channel 0
  auto channel = 0x00;  
  uint8_t config = mode | LO_HI | channel;
  debug("<PIT::set_mode> Setting mode %#x, config: %#x \n", mode, config);
  
  OS::outb(PIT_mode_register, config);
  current_mode_ = mode;

}

void PIT::set_freq(uint16_t freq_divider){    
  union{
    uint16_t whole;
    uint8_t part[2];
  }data{freq_divider};  

  // Send frequency hi/lo to PIT
  OS::outb(PIT_chan0, data.part[0]);
  OS::outb(PIT_chan0, data.part[1]);

  current_freq_divider_ = freq_divider;

}

void PIT::oneshot(uint16_t t){
    
  // Enable 1-shot mode
  set_mode(ONE_SHOT);  
  
  // Set a frequency for shot
  set_freq(t);
}

