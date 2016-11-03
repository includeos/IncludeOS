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

//#define DEBUG
//#define DEBUG2
#include <hw/ioport.hpp>
#include <hw/pit.hpp>
#include <os>
#include <hw/cpu_freq_sampling.hpp>
#include <kernel/irq_manager.hpp>

// Used for cpu frequency sampling
extern const uint16_t _cpu_sampling_freq_divider_;

namespace hw {

  constexpr MHz PIT::frequency_;

  // Bit 0-3: Mode 0 - "Interrupt on terminal count"
  // Bit 4-5: Both set, access mode "Lobyte / Hibyte"
  const uint8_t PIT_mode_register = 0x43;
  const uint8_t PIT_chan0 = 0x40;

  // PIT state
  PIT::Mode PIT::current_mode_ = NONE;
  PIT::Mode PIT::temp_mode_ = NONE;
  uint16_t PIT::current_freq_divider_ = 0;
  uint16_t PIT::temp_freq_divider_ = 0;

  uint64_t PIT::IRQ_counter_ = 0;

  // Time keeping
  uint64_t PIT::millisec_counter = 0;

  // The default recurring timer condition
  std::function<bool()> PIT::forever = []{ return true; };

  // Timer ID's
  uint32_t PIT::Timer::timers_count_ = 0;

  using namespace std::chrono;


  PIT::Timer::Timer(Type t, timeout_handler handler, std::chrono::milliseconds ms, repeat_condition cond)
    : type_{t}, id_{++timers_count_}, handler_{handler}, interval_{ms}, cond_{cond} {};


  void PIT::disable_regular_interrupts()
  {
    if (current_mode_ != ONE_SHOT)
      oneshot(1);
  }

  PIT::PIT() {}
  PIT::~PIT() {}

  double PIT::estimate_CPU_frequency(int samples) {

    debug("<PIT EstimateCPUFreq> Saving state: curr_freq_div %i \n",current_freq_divider_);
    // Save PIT-state
    temp_mode_ = current_mode_;
    temp_freq_divider_ = current_freq_divider_;

    auto prev_irq_handler = IRQ_manager::get().get_irq_handler(0);

    debug("<PIT EstimateCPUFreq> Sampling\n");
    IRQ_manager::get().set_irq_handler(0, cpu_sampling_irq_entry);

    // GO!
    set_mode(RATE_GEN);
    set_freq_divider(_cpu_sampling_freq_divider_);

    // BLOCKING call to external measurment.
    extern double calculate_cpu_frequency(int);
    double freq = calculate_cpu_frequency(samples);

    debug("<PIT EstimateCPUFreq> Done. Result: %f \n", _CPUFreq_);

    set_mode(temp_mode_);
    set_freq_divider(temp_freq_divider_);

    IRQ_manager::get().set_irq_handler(0, prev_irq_handler);
    
    return freq;
  }


  PIT::Timer_iterator PIT::start_timer(Timer t, std::chrono::milliseconds in_msecs){
    if (in_msecs < 1ms) panic("Can't wait less than 1 ms. ");

    if (current_mode_ != RATE_GEN) {
      set_mode(RATE_GEN);
      IRQ_manager::get().enable_irq(0);
    }

    if (current_freq_divider_ != millisec_interval)
      set_freq_divider(millisec_interval);

    auto cycles_pr_millisec = KHz(OS::cpu_freq());
    //debug("<PIT start_timer> CPU KHz: %f Cycles to wait: %f \n",cycles_pr_millisec.count(), cycles_pr_millisec.count() * in_msecs);

    auto ticks = in_msecs / KHz(current_frequency()).count();
    //debug("<PIT start_timer> PIT KHz: %f * %i = %f ms. \n",
    //KHz(current_frequency()).count(), (uint32_t)ticks.count(), ((uint32_t)ticks.count() * KHz(current_frequency()).count()));

    t.set_start(OS::cycles_since_boot());
    t.set_end(t.start() + uint64_t(cycles_pr_millisec.count() * in_msecs.count()));

    if (t.expired()) {
      debug ("<PIT start_timer> Timer was expired, restarting");
      t.restart();

    }

    uint64_t key = millisec_counter + ticks.count();

    // We could emplace, but the timer exists allready, and might be a reused one
    auto it = timers_.insert(std::make_pair(key, t));

    debug("<PIT start_timer> Key: %llu id: %i, t.cond()(): %s There are %i timers. \n",
          key, t.id(), t.cond()() ? "true" : "false", timers_.size());

    return it;
  }

  uint32_t  PIT::on_repeated_timeout(std::chrono::milliseconds ms, timeout_handler handler, repeat_condition cond){
    debug("<PIT repeated> setting a %i ms. repeating timer \n", (uint32_t)ms.count());

    Timer t(Timer::REPEAT_WHILE, handler, ms, cond);
    start_timer(t, ms);
    return t.id();
  };


  PIT::Timer_iterator PIT::on_timeout_ms(std::chrono::milliseconds msec, timeout_handler handler){
    Timer t(Timer::ONE_SHOT, handler, msec);

    debug("<PIT timeout> setting a %i ms. one-shot timer. Id: %i \n",
          (uint32_t)msec.count(), t.id());

    return start_timer(t, msec);

  };

  void PIT::stop_timer(PIT::Timer_iterator it) {
    if (not it->second.expired()) {
      it->second.stop();
      garbage_++;
    }
    debug("<PIT stop iterator> One-shot timer stopped. Garbage size: %i\n", garbage_);
  }

  void PIT::stop_timer(PIT::Timer::Id id) {
    auto it = std::find_if(timers_.begin(), timers_.end(), [id](auto it)->bool{ return it.second.id() == id; });
    debug("<PIT stop id> Stopping timer: id %i\n", it->second.id());
    if (not it->second.expired()) {
      it->second.stop();
      garbage_++;
    }
    debug("<PIT stop> Repeated timer stopped. Garbage size: %i\n", garbage_);
  }


  uint8_t PIT::read_back(uint8_t){
    const uint8_t READ_BACK_CMD = 0xc2;

    hw::outb(PIT_mode_register, READ_BACK_CMD );

    auto res = hw::inb(PIT_chan0);

    debug("STATUS: %#x \n", res);

    return res;

  }

  void PIT::irq_handler(){

    IRQ_counter_ ++;

    if (current_freq_divider_ == millisec_interval)
      millisec_counter++;

#ifdef DEBUG
    if (millisec_counter % 100 == 0)
      OS::rsprint(".");
#endif

    static std::vector<Timer_iterator> restart;
    restart.clear();

    // Iterate over expired timers (we break on the first non-expired)
    for (auto it = timers_.begin(); it != timers_.end(); it++) {

      // Map-keys are sorted. If this timer isn't expired, neither are the rest
      if (it->first > millisec_counter)
        break;

      debug ("\n**** Timer type %i, id: %i, key %llu expired. Running handler, in instance %p, millisec_counter %llu **** \n",
              it->second.type(), it->second.id(), it->first, this, millisec_counter);

      // Execute the handler
      it->second.handler()();

      // Re-queue repeating timers
      if (it->second.type() == Timer::REPEAT) {
        debug ("<Timer IRQ> REPEAT: Requeuing the timer \n");
        restart.push_back(it);

      }else if (it->second.type() == Timer::REPEAT_WHILE and it->second.cond()()) {
        debug ("<Timer IRQ> REPEAT_WHILE: Requeuing the timer COND \n");
        restart.push_back(it);
      }

      debug ("Timer done. Erasing.  \n");

      // Queue this timer for deletion (Escape iterator death by not deleting it now)
      stop_timer(it);


    }

    if (not garbage_) {
      debug2 ("Timers left: %i \n", timers_.size());

#ifdef DEBUG2
      for (auto t : timers_)
        debug2("Key: %llu , id: %i, Type: %i \n", t.first, t.second.id(), t.second.type());
#endif
      debug2("\n---------------------------\n\n");
    }

    for (auto t : restart) {
      start_timer(t->second, t->second.interval());
    }

    for (auto it = timers_.cbegin(); it != timers_.cend();  ) {
      debug2("Key: %llu , id: %i, Type: %i \n",
             it->first, it->second.id(), it->second.type());

      if (it->second.expired()) {
        debug("ERASE: Key: %llu , id: %i, Type: %i \n",
               it->first, it->second.id(), it->second.type());
        timers_.erase(it++);
        debug("Erased. \n");
      } else {
        ++it;
      }
    }

    garbage_ = 0;

    if (timers_.empty()) {
      debug("No more timers. Entering one-shot mode\n");
      disable_regular_interrupts();
    }


  }



  void PIT::init(){
    debug("<PIT> Initializing @ frequency: %16.16f MHz. Assigning myself to all timer interrupts.\n ", frequency().count());
    PIT::disable_regular_interrupts();
    // must be done to program IOAPIC to redirect to BSP LAPIC
    IRQ_manager::get().enable_irq(0);
    // register irq handler
    auto handler(IRQ_manager::irq_delegate{&instance(), &PIT::irq_handler});
    IRQ_manager::get().subscribe(0, handler);
  }

  void PIT::set_mode(Mode mode){
    // Channel is the last two bits in the PIT mode register
    // ...we always use channel 0
    auto channel = 0x00;
    uint8_t config = mode | LO_HI | channel;
    debug("<PIT::set_mode> Setting mode %#x, config: %#x \n", mode, config);

    hw::outb(PIT_mode_register, config);
    current_mode_ = mode;

  }

  void PIT::set_freq_divider(uint16_t freq_divider){
    union {
      uint16_t whole;
      uint8_t part[2];
    } data{freq_divider};

    // Send frequency hi/lo to PIT
    hw::outb(PIT_chan0, data.part[0]);
    hw::outb(PIT_chan0, data.part[1]);

    current_freq_divider_ = freq_divider;
  }

  void PIT::oneshot(uint16_t t){

    // Enable 1-shot mode
    set_mode(ONE_SHOT);

    // Set a frequency for shot
    set_freq_divider(t);
  }

} //< namespace hw
