#include <kernel/timer.hpp>

#include <map>
#include <vector>
#include <kernel/os.hpp>

typedef Timers::id_t       id_t;
typedef Timers::duration_t duration_t;
typedef Timers::handler_t  handler_t;

using namespace std::chrono;
void sched_timer(duration_t when, id_t id);
void stop_timer();

struct Timer
{
  Timer(duration_t p, handler_t cb)
    : period(p), callback(cb), deferred_destruct(false) {}
  
  bool is_alive() const noexcept {
    return deferred_destruct == false;
  }
  
  bool is_oneshot() const noexcept {
    return period.count() == 0;
  }
  
  duration_t period;
  handler_t  callback;
  bool deferred_destruct = false;
};

static std::vector<Timer>  timers;
static std::vector<id_t>   free_timers;
static bool   signal_ready = false;
static bool   is_running = false;
static double current_time;
static Timers::start_func_t arch_start_func;
static Timers::stop_func_t  arch_stop_func;

// timers sorted by timestamp
static std::multimap<duration_t, id_t> scheduled;

void Timers::init(const start_func_t& start, const stop_func_t& stop)
{
  // use uptime as position in timer system
  current_time = OS::uptime();
  // architecture specific start and stop functions
  arch_start_func = start;
  arch_stop_func  = stop;
}

void Timers::ready()
{
  signal_ready = true;
  // begin processing timers if any are queued
  if (is_running == false) {
    timers_handler();
  }
}

static id_t create_timer(
    duration_t when, duration_t period, const handler_t& handler)
{
  id_t id;
  
  if (free_timers.empty()) {
    id = timers.size();
    
    // occupy new slot
    timers.emplace_back(period, handler);
  }
  else {
    // get free timer slot
    id = free_timers.back();
    free_timers.pop_back();
    
    // occupy free slot
    new (&timers[id]) Timer(period, handler);
  }
  
  // immediately schedule the first timer
  sched_timer(when, id);
  return id;
}
id_t Timers::create(duration_t when, duration_t period, const handler_t& handler)
{
  return create_timer(when, period, handler);
}
id_t Timers::create(duration_t when, const handler_t& handler)
{
  return create_timer(when, milliseconds(0), handler);
}

void Timers::stop(id_t id)
{
  // mark as dead already
  timers[id].deferred_destruct = true;
  // note:
  // if the timer id is a legit "alive" timer,
  // then the timer MUST be visited at some point in interrupt handler,
  // and this will free the resources in the timers callback
}

/// time functions ///

inline auto now()
{
  return microseconds((int64_t)(OS::uptime() * 1000000.0));
}

/// scheduling ///

void Timers::timers_handler()
{
  while (!scheduled.empty())
  {
    auto it = scheduled.begin();
    auto when   = it->first;
    auto ts_now = now();
    id_t id = it->second;
    auto& timer = timers[id];
    
    if (ts_now >= when) {
      // erase immediately
      scheduled.erase(scheduled.begin());
      
      // only process timer if still alive
      if (timer.is_alive()) {
        // call the users callback function
        timer.callback();
        
        // if the timer died during callback, free its resources
        if (timer.deferred_destruct) {
          timer.deferred_destruct = false;
          timer.callback.reset();
        }
        else if (timer.period.count())
        {
          // if the timer is recurring, we will simply reschedule it
          sched_timer(timer.period, id);
        }
      }
      
    } else {
      // not yet time, so schedule it for later
      is_running = true;
      arch_start_func(when - ts_now);
      // exit early, because we have nothing more to do, and there is a deferred handler
      return;
    }
  }
  // lets assume the timer is not running anymore
  is_running = false;
  //arch_stop_func();
}
void sched_timer(duration_t when, id_t id)
{
  scheduled.insert(std::forward_as_tuple(now() + when, id));
  
  // If the hardware timer is not running, we have to start it somehow
  //  and timers_start() should program the hardware
  // This also optimizes the case where lots of timers are happening "now",
  //  typically the case for deferred actions, such as deferred kick
  if (is_running == false && signal_ready) {
    Timers::timers_handler();
  }
}
void stop_timer()
{
  arch_stop_func();
}
