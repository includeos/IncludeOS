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
  
  void reset() {
    callback.reset();
    deferred_destruct = false;
  }
  
  duration_t period;
  handler_t  callback;
  bool deferred_destruct = false;
};

/**
 * 1. There are no restrictions on when timers can be started or stopped
 * 2. A period of 0 means start a one-shot timer
 * 3. Each timer is a separate object living in a "fixed" vector
 * 4. A dead timer is simply a timer which has its handler reset, as well as
 *     having been removed from schedule
 * 5. No timer may be scheduled more than once at a time, as that will needlessly
 *     inflate the schedule container, as well as complicate stopping timers
 * 6. Free timer IDs are retrieved from a stack of free timer IDs (or through 
 *     expanding the "fixed" vector)
**/

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
id_t Timers::oneshot(duration_t when, const handler_t& handler)
{
  return create_timer(when, milliseconds(0), handler);
}
id_t Timers::periodic(duration_t when, duration_t period, const handler_t& handler)
{
  return create_timer(when, period, handler);
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
  // lets assume the timer is not running anymore
  is_running = false;
  
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
        timer.callback(id);
        
        // oneshot timers are automatically freed
        if (timer.deferred_destruct || timer.is_oneshot())
        {
          timer.reset();
        }
        else if (timer.is_oneshot() == false)
        {
          // if the timer is recurring, we will simply reschedule it
          sched_timer(timer.period, id);
        }
      } else {
        // timer was already dead
        timer.reset();
      }
      
    } else {
      // not yet time, so schedule it for later
      is_running = true;
      arch_start_func(when - ts_now);
      // exit early, because we have nothing more to do, and there is a deferred handler
      return;
    }
  }
  //arch_stop_func();
}
void sched_timer(duration_t when, id_t id)
{
  scheduled.insert(std::forward_as_tuple(now() + when, id));
  
  // dont start any hardware until after calibration
  if (!signal_ready) return;
  
  // if the hardware timer is not running, try starting it
  if (is_running == false) {
    Timers::timers_handler();
  }
  // or, if the scheduled timer is the new front, restart timer
  else if (scheduled.begin()->second == id) {
    Timers::timers_handler();
  }
}
void stop_timer()
{
  arch_stop_func();
}
